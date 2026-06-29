#!/usr/bin/env bash
#
# build_macos.sh — Build DROD natively on a modern Mac and package it as a
# self-contained, ad-hoc-signed .app (optionally a .dmg).
#
# Uses Apple Clang + Homebrew; all third-party dylibs are vendored into the
# bundle so it runs on a Mac with nothing installed. Builds native arm64 by
# default.
#
# The bundle's resource structure is assembled here. The public repo ships only
# a barebones drod5_0.dat (stub), the Help docs and the Licenses; the loose
# runtime assets (Fonts + a handful of UI Bitmaps) come from the private
# `drodData` repo. The list of loose assets is hardcoded below (LOOSE ASSETS) so
# this does not depend on any installed copy of DROD.
#
# Usage:
#   ./build_macos.sh [options]
#
# Options:
#   --assets <dir>     drodData checkout to take Fonts/Bitmaps from
#                      (default: auto-detect a sibling `drodData`).
#   --dat <file>       Copy this single .dat into the bundle (by its own name).
#   --dat-dir <dir>    Copy ALL *.dat from <dir> into the bundle (e.g. a Steam
#                      install's Data folder). Default: Master/Darwin/localdata if
#                      present. The repo only ships a barebones stub drod5_0.dat;
#                      real (copyrighted) data must come from here and is never
#                      committed. A Steam (TSS) build needs drod-tss5_0.dat.
#   --name "DROD TSS"  Bundle/app display name (default: "DROD TSS").
#   --build-type <custom|release|debug>   make build type (default: custom).
#   --output <dir>     Where to place the .app (default: ./custom/bin).
#   --universal        Build x86_64 + arm64 (UNIVERSAL_BUILD=1).
#   --steam            Steam build (-DSTEAMBUILD -DSTEAMBUILD_TSS_APP); links and
#                      vendors libsteam_api.dylib. Implies build type 'steam'.
#   --steam-sdk <dir>  Steamworks SDK path (default: ~/Developer/steam-sdk).
#   --steam-lib <file> libsteam_api.dylib to use (default: the SDK's osx one).
#   --steam-appid <id> Provision a local, gitignored Master/Darwin/steam_appid.txt
#                      with this id. Any Steam build then copies it into the bundle
#                      so it can run OUTSIDE the Steam client (dev testing only --
#                      this file must NOT be uploaded to the Steam depot). Remove
#                      Master/Darwin/steam_appid.txt to build a clean shippable app.
#   --dmg              Also produce a .dmg next to the .app.
#   --skip-deps        Don't run `brew install` (assume deps present).
#   -h, --help         Show this help.
#
set -euo pipefail

###############################################################################
# Locations
###############################################################################
DARWIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"   # Master/Darwin
REPO_DIR="$(cd "$DARWIN_DIR/../.." && pwd)"                  # repo root
METAKIT_DIR="$REPO_DIR/metakit"

# Defaults
APP_NAME="DROD TSS"
APP_VERSION="5.2.1"
DEPLOY="15.0"
BUILD_TYPE="custom"
OUTPUT_DIR="$DARWIN_DIR/custom/bin"
ASSETS_DIR=""
DAT_FILE=""
DAT_DIR=""           # dir of .dat files to copy in (default: Master/Darwin/localdata)
UNIVERSAL=0
MAKE_DMG=0
SKIP_DEPS=0
STEAM=0
STEAM_SDK="$HOME/Developer/steam-sdk"
STEAM_LIB=""        # default: the SDK's own redistributable dylib (version-matched)
STEAM_APPID=""      # if set, provisions the local (gitignored) steam_appid.txt
STEAM_APPID_FILE="$DARWIN_DIR/steam_appid.txt"   # gitignored; copied in for dev runs

# Homebrew build tools. The libraries themselves are built from source by
# build_deps.sh (universal, targeting an old macOS); curl + zlib come from the SDK.
BREW_DEPS=(cmake ninja pkg-config dylibbundler)

# ---- LOOSE ASSETS: the only resource files a bundle ships outside drod5_0.dat ----
# Fonts copied verbatim from <assets>/Fonts.
LOOSE_FONTS=(tomnr.ttf epilog.ttf)
# Bitmaps copied verbatim from <assets>/Bitmaps.
LOOSE_BITMAPS=(CaravelLogoSE.jpg CaravelLogoSW.jpg Cursor.png Door.png hourglass.png internet.png Icon.bmp Icon-32x32.bmp Icon-128x128.bmp)
# Bitmaps copied from <assets>/Bitmaps under a different name (parallel arrays; bash 3.2 safe).
LOOSE_BITMAP_RENAME_SRC=(Cursor16.png hourglass16.png internet16.png)
LOOSE_BITMAP_RENAME_DST=(Cursor16x16.png hourglass16x16.png internet16x16.png)

###############################################################################
# Helpers
###############################################################################
log()  { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33mwarning:\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31merror:\033[0m %s\n' "$*" >&2; exit 1; }

usage() { sed -n '2,/^set -euo/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//;/^set -euo/d'; }

###############################################################################
# Parse arguments
###############################################################################
while [[ $# -gt 0 ]]; do
	case "$1" in
		--assets)     ASSETS_DIR="${2:?}"; shift 2 ;;
		--dat)        DAT_FILE="${2:?}"; shift 2 ;;
		--dat-dir)    DAT_DIR="${2:?}"; shift 2 ;;
		--name)       APP_NAME="${2:?}"; shift 2 ;;
		--build-type) BUILD_TYPE="${2:?}"; shift 2 ;;
		--output)     OUTPUT_DIR="${2:?}"; shift 2 ;;
		--universal)  UNIVERSAL=1; shift ;;
		--steam)      STEAM=1; BUILD_TYPE="steam"; shift ;;
		--steam-sdk)  STEAM_SDK="${2:?}"; shift 2 ;;
		--steam-lib)  STEAM_LIB="${2:?}"; shift 2 ;;
		--steam-appid) STEAM_APPID="${2:?}"; shift 2 ;;
		--dmg)        MAKE_DMG=1; shift ;;
		--skip-deps)  SKIP_DEPS=1; shift ;;
		-h|--help)    usage; exit 0 ;;
		*)            die "unknown argument: $1 (see --help)" ;;
	esac
done

###############################################################################
# 0. Preflight
###############################################################################
[[ "$(uname -s)" == "Darwin" ]] || die "this script only runs on macOS"
command -v clang++ >/dev/null || die "Apple Clang not found — run: xcode-select --install"
command -v brew    >/dev/null || die "Homebrew not found — install from https://brew.sh"
HB_PREFIX="$(brew --prefix)"
log "Homebrew prefix: $HB_PREFIX"

MAKE_ARGS=()
[[ "$UNIVERSAL" -eq 1 ]] && MAKE_ARGS+=(UNIVERSAL_BUILD=1)

# Steam: validate the SDK, stage libsteam_api.dylib into the deps prefix (so the
# link finds it and dylibbundler vendors it), and pass the SDK path to make.
if [[ "$STEAM" -eq 1 ]]; then
	[[ -f "$STEAM_SDK/public/steam/steam_api.h" ]] || die "Steamworks SDK headers not found at $STEAM_SDK/public/steam (use --steam-sdk)"
	[[ -n "$STEAM_LIB" ]] || STEAM_LIB="$STEAM_SDK/redistributable_bin/osx/libsteam_api.dylib"
	[[ -f "$STEAM_LIB" ]] || die "libsteam_api.dylib not found at $STEAM_LIB (use --steam-lib)"
	MAKE_ARGS+=("STEAM_SDK=$STEAM_SDK")
	log "Steam build: SDK=$STEAM_SDK  lib=$STEAM_LIB"
	mkdir -p "$DARWIN_DIR/deps/lib"
	cp "$STEAM_LIB" "$DARWIN_DIR/deps/lib/libsteam_api.dylib"
	chmod +w "$DARWIN_DIR/deps/lib/libsteam_api.dylib"
	install_name_tool -id "@rpath/libsteam_api.dylib" "$DARWIN_DIR/deps/lib/libsteam_api.dylib"
	# Provision the local (gitignored) AppID file if --steam-appid was given.
	[[ -n "$STEAM_APPID" ]] && { echo "$STEAM_APPID" > "$STEAM_APPID_FILE"; log "Wrote $STEAM_APPID_FILE (gitignored, dev-only)"; }
fi

# Resolve assets dir (auto-detect a sibling drodData if not given)
if [[ -z "$ASSETS_DIR" ]]; then
	for cand in "$REPO_DIR/../drodData" "$REPO_DIR/../../drodData"; do
		[[ -d "$cand/Bitmaps" ]] && { ASSETS_DIR="$(cd "$cand" && pwd)"; break; }
	done
fi
[[ -n "$ASSETS_DIR" ]] && log "Assets (drodData): $ASSETS_DIR" || warn "No drodData assets found — Fonts/Bitmaps will be missing (see --assets)."

# Game data: default the .dat source to the local (gitignored) localdata dir.
[[ -z "$DAT_DIR" && -d "$DARWIN_DIR/localdata" ]] && DAT_DIR="$DARWIN_DIR/localdata"

###############################################################################
# 1. Dependencies (build tools + from-source universal libraries)
###############################################################################
if [[ "$SKIP_DEPS" -eq 0 ]]; then
	log "Checking Homebrew build tools..."
	missing=()
	for dep in "${BREW_DEPS[@]}"; do
		brew list --versions "$dep" >/dev/null 2>&1 || missing+=("$dep")
	done
	[[ ${#missing[@]} -gt 0 ]] && { log "brew install ${missing[*]}"; brew install "${missing[@]}"; }
	log "Building third-party dependencies from source (universal, macOS $DEPLOY+)..."
	"$DARWIN_DIR/build_deps.sh"
fi

###############################################################################
# 2. Build the drod executable
###############################################################################
log "Building drod ($BUILD_TYPE)..."
( cd "$DARWIN_DIR" && make ${MAKE_ARGS[@]+"${MAKE_ARGS[@]}"} "drod-$BUILD_TYPE" )
BUILT_BIN="$DARWIN_DIR/$BUILD_TYPE/bin/drod"
[[ -f "$BUILT_BIN" ]] || die "build did not produce $BUILT_BIN"
log "Built: $BUILT_BIN ($(lipo -archs "$BUILT_BIN"))"

###############################################################################
# 4. Assemble the .app: structure from the repo + loose assets from drodData
###############################################################################
APP="$OUTPUT_DIR/$APP_NAME.app"
RES_DATA="$APP/Contents/Resources/Data"
log "Assembling bundle: $APP"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Frameworks" "$RES_DATA"

# 4a. Executable, Info.plist, icon
cp "$BUILT_BIN" "$APP/Contents/MacOS/drod"
sed "s/%EXECUTABLE_NAME%/drod/; s/%VERSION%/$APP_VERSION/" \
	"$DARWIN_DIR/Info.plist.template" > "$APP/Contents/Info.plist"
cp "$DARWIN_DIR/DROD.icns" "$APP/Contents/Resources/DROD.icns"

# 4b. Game data (.dat). Priority: --dat-dir / localdata (all *.dat) and/or --dat
# (single file); otherwise the repo's barebones stub.
dat_copied=0
if [[ -n "$DAT_DIR" ]]; then
	[[ -d "$DAT_DIR" ]] || die "--dat-dir not found: $DAT_DIR"
	for d in "$DAT_DIR"/*.dat; do [[ -e "$d" ]] && { cp "$d" "$RES_DATA/"; dat_copied=1; }; done
	[[ $dat_copied -eq 1 ]] && log "Copied $(ls "$DAT_DIR"/*.dat 2>/dev/null | wc -l | tr -d ' ') .dat from $DAT_DIR"
fi
if [[ -n "$DAT_FILE" ]]; then cp "$DAT_FILE" "$RES_DATA/$(basename "$DAT_FILE")"; dat_copied=1; fi
if [[ $dat_copied -eq 0 ]]; then
	cp "$REPO_DIR/Data/drod5_0.dat" "$RES_DATA/drod5_0.dat"
	warn "Using the barebones stub drod5_0.dat — supply real data via --dat-dir or Master/Darwin/localdata."
	[[ "$STEAM" -eq 1 ]] && warn "A Steam (TSS) build needs drod-tss5_0.dat; the stub will not load."
fi

# 4b (cont). Help + Licenses from this repo.
[[ -d "$REPO_DIR/Data/Help" ]] && cp -R "$REPO_DIR/Data/Help" "$RES_DATA/Help"
[[ -d "$REPO_DIR/Licenses" ]] && { mkdir -p "$RES_DATA/Licenses"; cp -R "$REPO_DIR/Licenses/." "$RES_DATA/Licenses/"; }

# 4c. Loose assets from drodData (Fonts + the hardcoded Bitmaps list)
missing_assets=0
copy_asset() { # <src> <dst>
	if [[ -f "$1" ]]; then cp "$1" "$2"; else warn "missing loose asset: ${1##*/}"; missing_assets=1; fi
}
if [[ -n "$ASSETS_DIR" ]]; then
	mkdir -p "$RES_DATA/Fonts" "$RES_DATA/Bitmaps"
	for f in "${LOOSE_FONTS[@]}"; do copy_asset "$ASSETS_DIR/Fonts/$f" "$RES_DATA/Fonts/$f"; done
	for f in "${LOOSE_BITMAPS[@]}"; do copy_asset "$ASSETS_DIR/Bitmaps/$f" "$RES_DATA/Bitmaps/$f"; done
	for i in "${!LOOSE_BITMAP_RENAME_SRC[@]}"; do
		copy_asset "$ASSETS_DIR/Bitmaps/${LOOSE_BITMAP_RENAME_SRC[$i]}" "$RES_DATA/Bitmaps/${LOOSE_BITMAP_RENAME_DST[$i]}"
	done
else
	missing_assets=1
fi

# 4d. Steam: for dev runs outside the Steam client, copy the local (gitignored)
# steam_appid.txt next to the executable. This must NOT go into a Steam depot.
if [[ "$STEAM" -eq 1 && -f "$STEAM_APPID_FILE" ]]; then
	cp "$STEAM_APPID_FILE" "$APP/Contents/MacOS/steam_appid.txt"
	warn "Bundled steam_appid.txt (AppID $(tr -d '[:space:]' < "$STEAM_APPID_FILE")) for out-of-Steam dev testing — do NOT upload this file to the Steam depot."
fi

###############################################################################
# 5. Vendor all dependent dylibs into the bundle
###############################################################################
log "Vendoring dylibs with dylibbundler..."
# The deps are referenced as @rpath/...; -s tells dylibbundler where to find them.
dylibbundler -of -b \
	-x "$APP/Contents/MacOS/drod" \
	-d "$APP/Contents/Frameworks" \
	-p "@executable_path/../Frameworks" \
	-s "$DARWIN_DIR/deps/lib"

###############################################################################
# 6. Ad-hoc code sign (must be last; arm64 binaries must be signed to run)
###############################################################################
log "Ad-hoc code signing..."
codesign --force -s - "$APP/Contents/Frameworks/"*.dylib
codesign --force --deep -s - "$APP"
codesign --verify --deep "$APP" && log "Code signature OK"

# 6a. Sanity: no build-machine paths leaked into the bundle
if otool -L "$APP/Contents/MacOS/drod" "$APP/Contents/Frameworks/"*.dylib \
	| grep -qE "$HB_PREFIX|$DARWIN_DIR/deps|/usr/local/Cellar|/opt/sw"; then
	warn "Some libraries still reference build-machine paths — bundle may not be portable."
fi

log "Done: $APP"

###############################################################################
# 7. Optional .dmg
###############################################################################
if [[ "$MAKE_DMG" -eq 1 ]]; then
	DMG="$OUTPUT_DIR/$APP_NAME.dmg"
	log "Creating $DMG"
	rm -f "$DMG"
	hdiutil create -srcfolder "$APP" -volname "$APP_NAME" -ov -format UDZO "$DMG"
	log "Done: $DMG"
fi

###############################################################################
# 8. Summary
###############################################################################
echo
if [[ "$missing_assets" -ne 0 ]]; then
	warn "Loose Fonts/Bitmaps are missing — the app will show \"DROD couldn't load certain files\"."
	warn "Supply them with --assets <drodData checkout>. See CompilingDrod_macOS.md."
fi
cat <<EOF
Build complete.
  App: $APP
  Run: open "$APP"

The bundled drod5_0.dat is the barebones stub (the real game data is not
open-source). Distribution: the app is ad-hoc signed — on other Macs recipients
must right-click -> Open, or run:
  xattr -dr com.apple.quarantine "$APP_NAME.app"
EOF
