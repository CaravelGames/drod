#!/usr/bin/env bash
#
# build_macos.sh — Build DROD RPG natively on a modern Mac and package it as a
# self-contained, ad-hoc-signed .app (optionally a .dmg).
#
# Uses Apple Clang + Homebrew; all third-party dylibs are vendored into the
# bundle so it runs on a Mac with nothing installed. Builds native arm64 by
# default.
#
# DROD RPG reuses the main game's from-source dependencies: this script invokes
# the repo-root Master/Darwin/build_deps.sh and links against Master/Darwin/deps
# (the two games' dep sets are identical). It also shares BackEndLib/FrontEndLib
# and the metakit submodule with the repo root — see Config/Makefile.
#
# The bundle's resources come from three places (like the main DROD build):
#   * Help + Licenses:            this repo (drodrpg/Data/Help, drodrpg/Licenses).
#   * drodrpg2_0.dat (real data): the gitignored Master/Darwin/localdata/ dir
#                                 (the 219 MB build resource is not open-source;
#                                 the repo only ships a barebones stub). Tilesets,
#                                 music and sounds are baked into this .dat.
#   * Loose Fonts + UI Bitmaps:   the private `drodrpgData` repo (auto-detected as
#                                 a sibling, or via --assets). Only a small UI set
#                                 ships loose; see LOOSE ASSETS below.
#
# Usage:
#   ./build_macos.sh [options]
#
# Options:
#   --assets <dir>     drodrpgData checkout to take loose Fonts/Bitmaps from
#                      (default: auto-detect a sibling `drodrpgData`).
#   --dat <file>       Copy this single .dat into the bundle (by its own name).
#   --dat-dir <dir>    Copy drodrpg2_0.dat (+ add-on packs) from <dir>.
#                      Default: Master/Darwin/localdata.
#   --name <name>      Bundle/app display name (default: "DROD RPG 2").
#   --build-type <custom|release|debug>   make build type (default: custom).
#   --output <dir>     Where to place the .app (default: ./custom/bin).
#   --universal        Build x86_64 + arm64 (UNIVERSAL_BUILD=1).
#   --steam            Steam build. Implies build type 'steam'.
#   --steam-sdk <dir>  Steamworks SDK path (default: ~/Developer/steam-sdk).
#   --steam-lib <file> libsteam_api.dylib to use (default: the SDK's osx one).
#   --steam-appid      Write the AppID to Contents/MacOS/steam_appid.txt so the
#                      app can run OUTSIDE the Steam client (dev testing only --
#                      do NOT upload that file to the Steam depot).
#   --dmg              Also produce a .dmg next to the .app.
#   --skip-deps        Don't run brew install / build_deps.sh (assume deps present).
#   -h, --help         Show this help.
set -euo pipefail

###############################################################################
# Locations
###############################################################################
DARWIN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"   # drodrpg/Master/Darwin
DRODRPG_DIR="$(cd "$DARWIN_DIR/../.." && pwd)"               # drodrpg/
REPO_DIR="$(cd "$DRODRPG_DIR/.." && pwd)"                    # repo root
SHARED_DARWIN="$REPO_DIR/Master/Darwin"                     # main game's Darwin dir
DEPS_DIR="$SHARED_DARWIN/deps"                              # shared from-source deps

# Defaults
APP_NAME="DROD RPG 2"
APP_VERSION="2.0.9"
DEPLOY="11.0"
BUILD_TYPE="custom"
OUTPUT_DIR="$DARWIN_DIR/custom/bin"
ASSETS_DIR=""        # drodrpgData checkout for loose Fonts/Bitmaps (auto-detected)
DAT_FILE=""
DAT_DIR=""           # dir holding the real .dat (default: localdata)
UNIVERSAL=0
MAKE_DMG=0
SKIP_DEPS=0
STEAM=0
STEAM_APPID="3661610"
STEAM_INCLUDE_APPID=0
STEAM_SDK="$HOME/Developer/steam-sdk"
STEAM_LIB=""        # default: the SDK's own redistributable dylib

# The base .dat name the built binary looks for (wszUniqueResFile in DROD/Main.cpp).
DAT_BASE="drodrpg2_0"

# Homebrew build tools (the libraries themselves are built from source by the
# shared build_deps.sh). curl + zlib come from the macOS SDK.
BREW_DEPS=(cmake ninja pkg-config dylibbundler)

# ---- LOOSE ASSETS: the resource files the bundle ships outside drodrpg2_0.dat ----
# The 219 MB .dat bakes in the tilesets/music/sounds; only this small UI set + the
# two fonts ship loose, copied from <drodrpgData>. (RPG's title screen uses only
# the SW Caravel logo -- see DROD/TitleScreen.cpp -- so there is no CaravelLogoSE.)
LOOSE_FONTS=(tomnr.ttf epilog.ttf)
LOOSE_BITMAPS=(CaravelLogoSW.jpg Cursor.png Door.png hourglass.png internet.png)
# Bitmaps copied under a different name (parallel arrays; bash 3.2 safe).
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
		--steam-appid) STEAM_INCLUDE_APPID=1; shift ;;
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

# Steam: validate the SDK, stage libsteam_api.dylib into the shared deps prefix
# (so the link finds it and dylibbundler vendors it), and pass the SDK to make.
if [[ "$STEAM" -eq 1 ]]; then
	[[ -f "$STEAM_SDK/public/steam/steam_api.h" ]] || die "Steamworks SDK headers not found at $STEAM_SDK/public/steam (use --steam-sdk)"
	[[ -n "$STEAM_LIB" ]] || STEAM_LIB="$STEAM_SDK/redistributable_bin/osx/libsteam_api.dylib"
	[[ -f "$STEAM_LIB" ]] || die "libsteam_api.dylib not found at $STEAM_LIB (use --steam-lib)"
	MAKE_ARGS+=("STEAM_SDK=$STEAM_SDK")
	log "Steam build (AppID $STEAM_APPID)  SDK=$STEAM_SDK"
	mkdir -p "$DEPS_DIR/lib"
	cp "$STEAM_LIB" "$DEPS_DIR/lib/libsteam_api.dylib"
	chmod +w "$DEPS_DIR/lib/libsteam_api.dylib"
	install_name_tool -id "@rpath/libsteam_api.dylib" "$DEPS_DIR/lib/libsteam_api.dylib"
fi

# Real game data (.dat): default the source to the gitignored localdata dir.
if [[ -z "$DAT_DIR" && -d "$DARWIN_DIR/localdata" ]]; then
	DAT_DIR="$DARWIN_DIR/localdata"
fi

# Loose Fonts/Bitmaps: auto-detect a sibling drodrpgData checkout if not given.
if [[ -z "$ASSETS_DIR" ]]; then
	for cand in "$REPO_DIR/../drodrpgData" "$REPO_DIR/../../drodrpgData"; do
		[[ -d "$cand/Bitmaps" ]] && { ASSETS_DIR="$(cd "$cand" && pwd)"; break; }
	done
fi
[[ -n "$ASSETS_DIR" ]] && log "Assets (drodrpgData): $ASSETS_DIR" || warn "No drodrpgData assets found — Fonts/Bitmaps will be missing (see --assets)."

###############################################################################
# 1. Dependencies (build tools + shared from-source universal libraries)
###############################################################################
if [[ "$SKIP_DEPS" -eq 0 ]]; then
	log "Checking Homebrew build tools..."
	missing=()
	for dep in "${BREW_DEPS[@]}"; do
		brew list --versions "$dep" >/dev/null 2>&1 || missing+=("$dep")
	done
	[[ ${#missing[@]} -gt 0 ]] && { log "brew install ${missing[*]}"; brew install "${missing[@]}"; }
	log "Building shared third-party dependencies from source (universal, macOS $DEPLOY+)..."
	"$SHARED_DARWIN/build_deps.sh"
fi
[[ -f "$DEPS_DIR/lib/libmk4.a" ]] || die "shared deps not found at $DEPS_DIR (run without --skip-deps first)"

###############################################################################
# 2. Build the drod executable
###############################################################################
log "Building drod ($BUILD_TYPE)..."
( cd "$DARWIN_DIR" && make ${MAKE_ARGS[@]+"${MAKE_ARGS[@]}"} "drod-$BUILD_TYPE" )
BUILT_BIN="$DARWIN_DIR/$BUILD_TYPE/bin/drod"
[[ -f "$BUILT_BIN" ]] || die "build did not produce $BUILT_BIN"
log "Built: $BUILT_BIN ($(lipo -archs "$BUILT_BIN"))"

###############################################################################
# 3. Assemble the .app
###############################################################################
APP="$OUTPUT_DIR/$APP_NAME.app"
RES_DATA="$APP/Contents/Resources/Data"
log "Assembling bundle: $APP"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Frameworks" "$RES_DATA"

# 3a. Executable, Info.plist, icon
cp "$BUILT_BIN" "$APP/Contents/MacOS/drod"
sed "s/%EXECUTABLE_NAME%/drod/; s/%VERSION%/$APP_VERSION/; s/%MIN_OS%/$DEPLOY/" \
	"$DARWIN_DIR/Info.plist.template" > "$APP/Contents/Info.plist"
if [[ -f "$DARWIN_DIR/assets/DROD.icns" ]]; then
	cp "$DARWIN_DIR/assets/DROD.icns" "$APP/Contents/Resources/DROD.icns"
else
	warn "assets/DROD.icns not found — the app will have no icon."
fi

# 3b. Help + Licenses from this repo.
[[ -d "$DRODRPG_DIR/Data/Help" ]] && cp -R "$DRODRPG_DIR/Data/Help" "$RES_DATA/Help"
[[ -d "$DRODRPG_DIR/Licenses" ]] && { mkdir -p "$RES_DATA/Licenses"; cp -R "$DRODRPG_DIR/Licenses/." "$RES_DATA/Licenses/"; }

# 3c. Game data (.dat). Copy ONLY this build's data set — the base file the binary
# loads ($DAT_BASE.dat, per wszUniqueResFile) plus any incremental add-on packs
# ($DAT_BASE_NN.dat). --dat copies one explicit file; otherwise the barebones stub.
missing_assets=0
dat_copied=0
if [[ -n "$DAT_DIR" ]]; then
	[[ -d "$DAT_DIR" ]] || die "--dat-dir not found: $DAT_DIR"
	for d in "$DAT_DIR/$DAT_BASE.dat" "$DAT_DIR/${DAT_BASE}_"*.dat; do
		[[ -e "$d" ]] && { cp "$d" "$RES_DATA/"; dat_copied=1; }
	done
	[[ $dat_copied -eq 1 ]] && log "Copied $DAT_BASE data set from $DAT_DIR"
fi
if [[ -n "$DAT_FILE" ]]; then cp "$DAT_FILE" "$RES_DATA/$(basename "$DAT_FILE")"; dat_copied=1; fi
if [[ $dat_copied -eq 0 ]]; then
	cp "$DRODRPG_DIR/Data/$DAT_BASE.dat" "$RES_DATA/$DAT_BASE.dat"
	warn "Using the barebones stub $DAT_BASE.dat — supply real data via --dat-dir or Master/Darwin/localdata."
	missing_assets=1
fi

# 3d. Loose Fonts + UI Bitmaps from drodrpgData (the tilesets/music/sounds are
# baked into $DAT_BASE.dat, so only this small UI set ships loose).
copy_asset() { # <src> <dst>
	if [[ -f "$1" ]]; then cp "$1" "$2"; else warn "missing loose asset: ${1##*/}"; missing_assets=1; fi
}
if [[ -n "$ASSETS_DIR" ]]; then
	mkdir -p "$RES_DATA/Fonts" "$RES_DATA/Bitmaps"
	for f in "${LOOSE_FONTS[@]}"; do copy_asset "$ASSETS_DIR/Fonts/$f" "$RES_DATA/Fonts/$f"; done
	for f in "${LOOSE_BITMAPS[@]}"; do copy_asset "$ASSETS_DIR/Bitmaps/$f" "$RES_DATA/Bitmaps/$f"; done
	# The loader expects the *16x16.png spellings for these three.
	for i in "${!LOOSE_BITMAP_RENAME_SRC[@]}"; do
		copy_asset "$ASSETS_DIR/Bitmaps/${LOOSE_BITMAP_RENAME_SRC[$i]}" "$RES_DATA/Bitmaps/${LOOSE_BITMAP_RENAME_DST[$i]}"
	done
else
	missing_assets=1
fi

# 3e. App icon bitmap. RPG loads Bitmaps/Icon.bmp as the window icon (and an
# optional Bitmaps/Icon128x128.bmp), see DROD/Main.cpp. These come from assets/
# (the .icns's companion), not from drodrpgData, so a build without drodrpgData
# still gets the window icon.
if compgen -G "$DARWIN_DIR/assets/*.bmp" > /dev/null; then
	mkdir -p "$RES_DATA/Bitmaps"
	cp "$DARWIN_DIR/assets/"*.bmp "$RES_DATA/Bitmaps/"
fi

# 3f. Steam: write the AppID next to the executable so the app can run outside
# the Steam client (dev testing). Omitted unless --steam-appid; never ship it.
if [[ "$STEAM" -eq 1 && "$STEAM_INCLUDE_APPID" -eq 1 ]]; then
	echo "$STEAM_APPID" > "$APP/Contents/MacOS/steam_appid.txt"
	warn "Bundled steam_appid.txt (AppID $STEAM_APPID) for out-of-Steam dev testing — do NOT upload it to the Steam depot."
fi

###############################################################################
# 4. Vendor all dependent dylibs into the bundle
###############################################################################
log "Vendoring dylibs with dylibbundler..."
dylibbundler -of -b \
	-x "$APP/Contents/MacOS/drod" \
	-d "$APP/Contents/Frameworks" \
	-p "@executable_path/../Frameworks" \
	-s "$DEPS_DIR/lib"

###############################################################################
# 5. Ad-hoc code sign (must be last; arm64 binaries must be signed to run)
###############################################################################
log "Ad-hoc code signing..."
# Sign inside-out (nested code first, bundle last), not with --deep: --deep leaves
# an invalid seal over libsteam_api.dylib, which Valve ships hardened-runtime with
# a designated requirement. Verify with --strict to catch a bad seal.
codesign --force -s - "$APP/Contents/Frameworks/"*.dylib
codesign --force -s - "$APP/Contents/MacOS/drod"
codesign --force -s - "$APP"
codesign --verify --deep --strict "$APP" && log "Code signature OK"

# 5a. Sanity: no build-machine paths leaked into the bundle
if otool -L "$APP/Contents/MacOS/drod" "$APP/Contents/Frameworks/"*.dylib \
	| grep -qE "$HB_PREFIX|$DEPS_DIR|/usr/local/Cellar|/opt/sw"; then
	warn "Some libraries still reference build-machine paths — bundle may not be portable."
fi

log "Done: $APP"

###############################################################################
# 6. Optional .dmg
###############################################################################
if [[ "$MAKE_DMG" -eq 1 ]]; then
	DMG="$OUTPUT_DIR/$APP_NAME.dmg"
	log "Creating $DMG"
	rm -f "$DMG"
	hdiutil create -srcfolder "$APP" -volname "$APP_NAME" -ov -format UDZO "$DMG"
	log "Done: $DMG"
fi

###############################################################################
# 7. Summary
###############################################################################
echo
if [[ "$missing_assets" -ne 0 ]]; then
	warn "Real game data/assets are missing — the app will show \"DROD couldn't load certain files\"."
	warn "Supply the real drodrpg2_0.dat in Master/Darwin/localdata and a drodrpgData"
	warn "checkout for the loose Fonts/Bitmaps (auto-detected, or via --assets)."
fi
cat <<EOF
Build complete.
  App: $APP
  Run: open "$APP"

Distribution: the app is ad-hoc signed — on other Macs recipients
must right-click -> Open, or run:
  xattr -dr com.apple.quarantine "$APP_NAME.app"
EOF
