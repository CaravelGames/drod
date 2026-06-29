# Building DROD on macOS

The macOS build uses **Apple Clang + Homebrew** and produces a self-contained,
ad-hoc-signed `.app` (optionally a `.dmg`) with all third-party libraries vendored
inside it, so it runs on a Mac with nothing installed. It builds native **arm64**
(Apple Silicon).

## Prerequisites

1. **Xcode Command Line Tools** (provides `clang`, `make`, `git`):
   ```
   xcode-select --install
   ```
2. **Homebrew** — https://brew.sh
3. The **metakit** submodule (the build script initializes it automatically, or):
   ```
   git submodule update --init metakit
   ```

The build script installs the required Homebrew formulae itself: `sdl2 sdl2_mixer
sdl2_ttf libpng jpeg-turbo libvorbis libogg theora expat freetype jsoncpp
dylibbundler`. (`curl` and `zlib` come from the macOS SDK.)

## Quick start

```
cd Master/Darwin
./build_macos.sh
```

This builds `drod`, assembles `DROD TSS.app` under `Master/Darwin/custom/bin/`,
vendors all dylibs, and ad-hoc signs it. Add `--dmg` to also produce a disk image.
Run `./build_macos.sh --help` for all options.

To build just the binary (no bundle):
```
cd Master/Darwin
make drod-custom          # or: make drod-release / make drod-debug
```

## Game assets (important)

A runnable bundle's `Contents/Resources/Data` needs `drod5_0.dat`, the loose UI
`Bitmaps/` and `Fonts/`, plus `Help/` and `Licenses/`. **This public repository
does not contain all of them:**

| Asset | Source |
|-------|--------|
| `drod5_0.dat` | **barebones stub** in `Data/drod5_0.dat` (the real, full data is not open-source) |
| `Help/`, `Licenses/` | this repo (`Data/Help`, `Licenses/`) |
| `Fonts/` (`tomnr.ttf`, `epilog.ttf`), loose `Bitmaps/` | the private **`drodData`** repo (`CaravelGames/drodData`) |

`build_macos.sh` populates the structure from this repo and overlays the loose
Fonts/Bitmaps from a `drodData` checkout. It auto-detects a sibling `drodData`
directory, or point it explicitly:

```
./build_macos.sh --assets /path/to/drodData
```

The exact list of loose files copied is hardcoded near the top of `build_macos.sh`
(`LOOSE_FONTS` / `LOOSE_BITMAPS`). Without `drodData`, the script still builds a
correctly-structured bundle but warns that Fonts/Bitmaps are missing — the app
will then show "DROD couldn't load certain files."

To run with the **full** game content, supply an official `drod5_0.dat`:
```
./build_macos.sh --assets /path/to/drodData --dat /path/to/official/drod5_0.dat
```

## Distribution / Gatekeeper

The app is **ad-hoc signed** (no Apple Developer ID). On another Mac, Gatekeeper
blocks the first launch — recipients must **right-click → Open**, or clear the
quarantine attribute:
```
xattr -dr com.apple.quarantine "DROD TSS.app"
```

## Universal (Intel + Apple Silicon), and older macOS

`./build_macos.sh --universal` (or `make UNIVERSAL_BUILD=1`) produces a universal
(`x86_64 + arm64`) binary that runs on **macOS 15+**, including Intel Macs.

This works because the third-party dependencies are not taken from Homebrew (whose
dylibs target the build machine's current macOS) but are **built from source as
universal dylibs targeting macOS 15** by `build_deps.sh`, which `build_macos.sh`
runs automatically. Real SDL2 is used (not Homebrew's sdl2-compat shim, so no
SDL3), and SDL2_mixer is built lean (Ogg/Vorbis + WAV + built-in mp3 only).

The deployment target is `MACOSX_VERSION_MIN = 15.0` in `Config` (and the default
in `build_deps.sh`); change both to target a different floor. Note: a build can
only be verified on the build machine's own macOS — test the resulting `.app` on
an actual macOS 15 / Intel Mac before relying on it there.

## Steam build

```
./build_macos.sh --steam --universal --steam-sdk /path/to/steamworks-sdk
```

This builds with `-DSTEAMBUILD -DSTEAMBUILD_TSS_APP` (the TSS Steam app: data file
`drod-tss5_0.dat`, game name `drod-tss`) and links `libsteam_api`. Details:
- `--steam-sdk` points at an unpacked Steamworks SDK (default `~/Developer/steam-sdk`);
  its `public/steam` headers are added to the include path.
- `libsteam_api.dylib` is taken from the SDK's `redistributable_bin/osx/` (override
  with `--steam-lib`), staged into `deps/lib`, vendored into the bundle, and signed.
- A Steam build is normally launched through the Steam client (which supplies the
  app id), so the shipped bundle contains no `steam_appid.txt`.
- To run a Steam build **outside** Steam for local testing, provision a gitignored
  AppID file once: `./build_macos.sh --steam --steam-appid <id> …`. That writes
  `Master/Darwin/steam_appid.txt` (gitignored), and every subsequent Steam build
  copies it next to the executable so `SteamAPI_Init` works without Steam launching
  it. **Never upload that file to the Steam depot** — delete
  `Master/Darwin/steam_appid.txt` to build a clean shippable app.
- Just the binary: `make drod-steam STEAM_SDK=/path/to/steamworks-sdk`.
