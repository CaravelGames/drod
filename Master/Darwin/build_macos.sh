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
#   --dat-dir <dir>    Copy ALL *.dat from <dir> into the bundle. Default for a
#                      Steam build: Master/Darwin/localdata/<App> (TSS or GatEB).
#                      Real (copyrighted) data lives there and is never committed.
#   --name <name>      Bundle/app display name (default: per game/Steam variant,
#                      else "DROD TSS").
#   --game <kdd|jtrh|tcb|gateb>   Build a STANDALONE older game (not Steam):
#                      kdd   = King Dugan's Dungeon  (-DKDD_STANDALONE,  drod-kdd5_0.dat)
#                      jtrh  = Journey to Rooted Hold (-DJTRH_STANDALONE, drod-jtrh5_0.dat)
#                      tcb   = The City Beneath       (-DTCB_STANDALONE,  drod-tcb5_0.dat)
#                      gateb = Gunthro and the Epic Blunder (-DGATEB_STANDALONE, drod-gateb5_0.dat)
#                      Uses icon assets/<App> and data localdata/<App>. Cannot be
#                      combined with --steam.
#   --demo             Build the TSS demo: the plain TSS binary branded "DROD TSS
#                      Demo", shipping the demo drod5_0.dat from localdata/TSS-Demo.
#                      The engine shows it as a demo automatically (its hold has
#                      <=50 entrances) — no compile flag involved. TSS-standalone
#                      only; not with --game or --steam.
#   --build-type <custom|release|debug>   make build type (default: custom).
#   --output <dir>     Where to place the .app (default: ./custom/bin).
#   --universal        Build x86_64 + arm64 (UNIVERSAL_BUILD=1).
#   --steam            Steam build (variant 'tss' by default). Implies build type 'steam'.
#   --steam-variant <tss|gateb>  Which Steam app to build (implies --steam):
#                      tss   = DROD: The Second Sky (AppID 351320, -DSTEAMBUILD_TSS_APP,
#                              data drod-tss5_0.dat, icon assets/TSS)
#                      gateb = Gunthro and the Epic Blunder (AppID 314330, no TSS_APP,
#                              data drod5_0.dat, icon assets/GatEB)
#   --steam-sdk <dir>  Steamworks SDK path (default: ~/Developer/steam-sdk).
#   --steam-lib <file> libsteam_api.dylib to use (default: the SDK's osx one).
#   --steam-appid      Write the variant's AppID to Contents/MacOS/steam_appid.txt so
#                      the app can run OUTSIDE the Steam client (dev testing only --
#                      do NOT upload that file to the Steam depot). Omit for a clean app.
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
APP_NAME=""          # resolved from the Steam variant (or "DROD TSS") unless --name given
NAME_SET=0
APP_VERSION="5.2.1"
DEPLOY="11.0"
BUILD_TYPE="custom"
OUTPUT_DIR="$DARWIN_DIR/custom/bin"
ASSETS_DIR=""
DAT_FILE=""
DAT_DIR=""           # dir of .dat files to copy in (default: localdata/<App>)
UNIVERSAL=0
MAKE_DMG=0
SKIP_DEPS=0
GAME=""              # kdd | jtrh | tcb | gateb — standalone game (see the game table below)
DEMO=0               # 1 = TSS demo build (plain build + demo data, branded "DROD TSS Demo")
STEAM=0
STEAM_VARIANT="tss"  # tss | gateb (see the variant table below)
STEAM_INCLUDE_APPID=0
STEAM_SDK="$HOME/Developer/steam-sdk"
STEAM_LIB=""        # default: the SDK's own redistributable dylib (version-matched)

# Homebrew build tools. The libraries themselves are built from source by
# build_deps.sh (universal, targeting an old macOS); curl + zlib come from the SDK.
BREW_DEPS=(cmake ninja pkg-config dylibbundler)

# ---- LOOSE ASSETS: the resource files a bundle ships outside drod5_0.dat ----
# Fonts copied verbatim from <drodData>/Fonts.
LOOSE_FONTS=(tomnr.ttf epilog.ttf)
# SHARED bitmaps from <drodData>/Bitmaps (used by every game). The game-branded
# icon bitmaps (Icon*.bmp) are NOT here -- they come per-app from
# Master/Darwin/assets/<App>/ so e.g. GatEB never gets TSS's Icon-32x32.bmp.
LOOSE_BITMAPS=(CaravelLogoSE.jpg CaravelLogoSW.jpg Cursor.png Door.png hourglass.png internet.png)
# Shared bitmaps copied under a different name (parallel arrays; bash 3.2 safe).
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
		--name)       APP_NAME="${2:?}"; NAME_SET=1; shift 2 ;;
		--game)       GAME="${2:?}"; shift 2 ;;
		--demo)       DEMO=1; shift ;;
		--build-type) BUILD_TYPE="${2:?}"; shift 2 ;;
		--output)     OUTPUT_DIR="${2:?}"; shift 2 ;;
		--universal)  UNIVERSAL=1; shift ;;
		--steam)      STEAM=1; BUILD_TYPE="steam"; shift ;;
		--steam-variant) STEAM=1; BUILD_TYPE="steam"; STEAM_VARIANT="${2:?}"; shift 2 ;;
		--steam-sdk)  STEAM_SDK="${2:?}"; shift 2 ;;
		--steam-lib)  STEAM_LIB="${2:?}"; shift 2 ;;
		--steam-appid) STEAM_INCLUDE_APPID=1; shift ;;
		--dmg)        MAKE_DMG=1; shift ;;
		--skip-deps)  SKIP_DEPS=1; shift ;;
		-h|--help)    usage; exit 0 ;;
		*)            die "unknown argument: $1 (see --help)" ;;
	esac
done

[[ -n "$GAME" && "$STEAM" -eq 1 ]] && die "--game (standalone) cannot be combined with --steam"
[[ "$DEMO" -eq 1 && ( -n "$GAME" || "$STEAM" -eq 1 ) ]] && die "--demo is TSS-standalone only (not with --game or --steam)"

###############################################################################
# Standalone game table  (older games built from the TSS engine, no Steam)
#   game -> app folder/name suffix | -D<GAME>_STANDALONE (set in Config via GAME)
###############################################################################
GAME_APP=""
if [[ -n "$GAME" ]]; then
	case "$GAME" in
		kdd)   GAME_APP="KDD"   ;;
		jtrh)  GAME_APP="JtRH"  ;;
		tcb)   GAME_APP="TCB"   ;;
		gateb) GAME_APP="GatEB" ;;
		*)     die "unknown --game '$GAME' (expected kdd|jtrh|tcb|gateb)" ;;
	esac
	[[ "$NAME_SET" -eq 1 ]] || APP_NAME="DROD $GAME_APP"
fi

###############################################################################
# Steam variant table  (the AppIDs live here for easy switching)
#   variant -> app name | Steam AppID | STEAMBUILD_TSS_APP?
###############################################################################
STEAM_APP=""; STEAM_APPID=""; STEAM_TSS=""
if [[ "$STEAM" -eq 1 ]]; then
	case "$STEAM_VARIANT" in
		tss)   STEAM_APP="TSS";   STEAM_APPID="351320"; STEAM_TSS="1" ;;
		gateb) STEAM_APP="GatEB"; STEAM_APPID="314330"; STEAM_TSS=""  ;;
		*)     die "unknown --steam-variant '$STEAM_VARIANT' (expected tss|gateb)" ;;
	esac
	[[ "$NAME_SET" -eq 1 ]] || APP_NAME="DROD $STEAM_APP"
fi
# Demo: the plain TSS build, just branded differently (compile-identical to full).
[[ "$DEMO" -eq 1 && "$NAME_SET" -eq 0 ]] && APP_NAME="DROD TSS Demo"
[[ -n "$APP_NAME" ]] || APP_NAME="DROD TSS"          # plain non-Steam default
ICON_APP="${GAME_APP:-${STEAM_APP:-TSS}}"            # standalone game, else Steam app, else TSS

# The base .dat name the built binary looks for (must match wszUniqueResFile in
# DROD/Main.cpp). Used below to copy ONLY this build's data set — so a shared
# localdata/<App> folder can hold several games' dats without mixing them.
DAT_BASE="drod5_0"                                   # plain / Steam GatEB default
[[ "$STEAM" -eq 1 && -n "$STEAM_TSS" ]] && DAT_BASE="drod-tss5_0"
case "$GAME" in
	kdd)   DAT_BASE="drod-kdd5_0"   ;;
	jtrh)  DAT_BASE="drod-jtrh5_0"  ;;
	tcb)   DAT_BASE="drod-tcb5_0"   ;;
	gateb) DAT_BASE="drod-gateb5_0" ;;
esac

# Identity of the objects in the active build dir. make can't see a changed -D
# flag, so we record this and wipe the build dir when it changes (see below).
# Steam (tss/gateb) and standalone (game/default) never share a build dir, so a
# bare variant/game name is a sufficient key.
if [[ "$STEAM" -eq 1 ]]; then VARIANT_KEY="$STEAM_VARIANT"; else VARIANT_KEY="${GAME:-default}"; fi

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
[[ -n "$GAME" ]] && MAKE_ARGS+=("GAME=$GAME")   # -> -D<GAME>_STANDALONE (see Config)

# make can't see a changed -D flag, so wipe the active build dir when the game /
# Steam variant it was last built for differs (or is unknown) to avoid reusing
# stale objects. Applies to every build dir: steam/ (tss<->gateb) and
# custom|release|debug/ (default<->kdd<->jtrh<->...).
if [[ -d "$DARWIN_DIR/$BUILD_TYPE" && "$(cat "$DARWIN_DIR/$BUILD_TYPE/.variant" 2>/dev/null)" != "$VARIANT_KEY" ]]; then
	log "Build variant changed -> cleaning $BUILD_TYPE/ for a correct rebuild ($VARIANT_KEY)"
	chmod -R u+w "$DARWIN_DIR/$BUILD_TYPE" 2>/dev/null || true
	rm -rf "$DARWIN_DIR/$BUILD_TYPE"
fi

# Steam: validate the SDK, stage libsteam_api.dylib into the deps prefix (so the
# link finds it and dylibbundler vendors it), and pass the SDK path to make.
if [[ "$STEAM" -eq 1 ]]; then
	[[ -f "$STEAM_SDK/public/steam/steam_api.h" ]] || die "Steamworks SDK headers not found at $STEAM_SDK/public/steam (use --steam-sdk)"
	[[ -n "$STEAM_LIB" ]] || STEAM_LIB="$STEAM_SDK/redistributable_bin/osx/libsteam_api.dylib"
	[[ -f "$STEAM_LIB" ]] || die "libsteam_api.dylib not found at $STEAM_LIB (use --steam-lib)"
	MAKE_ARGS+=("STEAM_SDK=$STEAM_SDK")
	[[ -n "$STEAM_TSS" ]] && MAKE_ARGS+=(STEAM_TSS=1)
	log "Steam build: variant=$STEAM_VARIANT ($STEAM_APP, AppID $STEAM_APPID)  SDK=$STEAM_SDK"
	mkdir -p "$DARWIN_DIR/deps/lib"
	cp "$STEAM_LIB" "$DARWIN_DIR/deps/lib/libsteam_api.dylib"
	chmod +w "$DARWIN_DIR/deps/lib/libsteam_api.dylib"
	install_name_tool -id "@rpath/libsteam_api.dylib" "$DARWIN_DIR/deps/lib/libsteam_api.dylib"
	# (steam/ is cleaned above by the shared build-variant guard when tss<->gateb changes)
fi

# Resolve assets dir (auto-detect a sibling drodData if not given)
if [[ -z "$ASSETS_DIR" ]]; then
	for cand in "$REPO_DIR/../drodData" "$REPO_DIR/../../drodData"; do
		[[ -d "$cand/Bitmaps" ]] && { ASSETS_DIR="$(cd "$cand" && pwd)"; break; }
	done
fi
[[ -n "$ASSETS_DIR" ]] && log "Assets (drodData): $ASSETS_DIR" || warn "No drodData assets found — Fonts/Bitmaps will be missing (see --assets)."

# Game data: default the .dat source to the app's local (gitignored) data dir.
if [[ -z "$DAT_DIR" ]]; then
	if [[ "$DEMO" -eq 1 && -d "$DARWIN_DIR/localdata/TSS-Demo" ]]; then
		DAT_DIR="$DARWIN_DIR/localdata/TSS-Demo"   # demo drod5_0.dat, kept apart from Steam localdata/TSS
	elif [[ -n "$GAME_APP" && -d "$DARWIN_DIR/localdata/$GAME_APP" ]]; then
		DAT_DIR="$DARWIN_DIR/localdata/$GAME_APP"
	elif [[ "$STEAM" -eq 1 && -d "$DARWIN_DIR/localdata/$STEAM_APP" ]]; then
		DAT_DIR="$DARWIN_DIR/localdata/$STEAM_APP"
	elif ls "$DARWIN_DIR/localdata"/*.dat >/dev/null 2>&1; then
		DAT_DIR="$DARWIN_DIR/localdata"
	fi
fi

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
# Record which game/variant these objects were built with (see the clean above).
echo "$VARIANT_KEY" > "$DARWIN_DIR/$BUILD_TYPE/.variant"

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
sed "s/%EXECUTABLE_NAME%/drod/; s/%VERSION%/$APP_VERSION/; s/%MIN_OS%/$DEPLOY/" \
	"$DARWIN_DIR/Info.plist.template" > "$APP/Contents/Info.plist"
# Icon: the app's own .icns, falling back to TSS's if a new game's art isn't in
# place yet (so a data-only test build still completes).
if [[ -f "$DARWIN_DIR/assets/$ICON_APP/DROD.icns" ]]; then
	cp "$DARWIN_DIR/assets/$ICON_APP/DROD.icns" "$APP/Contents/Resources/DROD.icns"
else
	warn "assets/$ICON_APP/DROD.icns not found — using the TSS icon as a placeholder."
	cp "$DARWIN_DIR/assets/TSS/DROD.icns" "$APP/Contents/Resources/DROD.icns"
fi

# 4b. Game data (.dat). Copy ONLY this build's data set — the base file the
# binary loads ($DAT_BASE.dat, per wszUniqueResFile) plus its own incremental
# add-on packs ($DAT_BASE_NN.dat). This keeps a shared localdata/<App> folder
# (which may also hold another game's dats) from leaking mismatched data into the
# bundle. --dat copies one explicit file verbatim; otherwise the barebones stub.
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
	cp "$REPO_DIR/Data/drod5_0.dat" "$RES_DATA/drod5_0.dat"
	warn "Using the barebones stub drod5_0.dat — supply real data via --dat-dir or Master/Darwin/localdata."
	[[ "$DAT_BASE" != "drod5_0" ]] && warn "This build loads $DAT_BASE.dat; the stub will not load."
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

# 4d. Per-app branding overlay: the app's game-branded bitmaps (CaravelLogoSW,
# Icon*.bmp, ...) from assets/<App>/ — these are the only source for those files,
# so a variant only ships its own (e.g. GatEB gets its Icon-128x128.bmp, not TSS's).
if [[ -d "$DARWIN_DIR/assets/$ICON_APP" ]]; then
	mkdir -p "$RES_DATA/Bitmaps"
	for b in "$DARWIN_DIR/assets/$ICON_APP"/*.bmp "$DARWIN_DIR/assets/$ICON_APP"/*.png "$DARWIN_DIR/assets/$ICON_APP"/*.jpg; do
		[[ -e "$b" ]] && cp "$b" "$RES_DATA/Bitmaps/"
	done
fi

# 4e. Steam: write the variant's AppID next to the executable so the app can run
# outside the Steam client (dev testing). Omitted unless --steam-appid; never ship it.
if [[ "$STEAM" -eq 1 && "$STEAM_INCLUDE_APPID" -eq 1 ]]; then
	echo "$STEAM_APPID" > "$APP/Contents/MacOS/steam_appid.txt"
	warn "Bundled steam_appid.txt (AppID $STEAM_APPID) for out-of-Steam dev testing — do NOT upload it to the Steam depot."
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
# Sign inside-out (nested code first, bundle last), not with --deep: --deep leaves
# an invalid seal over libsteam_api.dylib, which Valve ships hardened-runtime with
# a designated requirement. Verify with --strict to catch a bad seal.
codesign --force -s - "$APP/Contents/Frameworks/"*.dylib
codesign --force -s - "$APP/Contents/MacOS/drod"
codesign --force -s - "$APP"
codesign --verify --deep --strict "$APP" && log "Code signature OK"

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

EOF
if [[ "$dat_copied" -eq 0 ]]; then
	cat <<EOF
The bundled drod5_0.dat is the barebones stub (the real game data is not
open-source) — the app will not load a hold until you supply $DAT_BASE.dat.
EOF
fi
cat <<EOF
Distribution: the app is ad-hoc signed — on other Macs recipients
must right-click -> Open, or run:
  xattr -dr com.apple.quarantine "$APP_NAME.app"
EOF
