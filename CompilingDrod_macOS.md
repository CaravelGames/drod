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

There are two Steam apps built from this engine, selected with `--steam-variant`:

| variant | game | AppID | define | data file | icon |
|---------|------|-------|--------|-----------|------|
| `tss` (default) | DROD: The Second Sky | 351320 | `-DSTEAMBUILD_TSS_APP` | `drod-tss5_0.dat` | `assets/TSS` |
| `gateb` | Gunthro and the Epic Blunder | 314330 | (none) | `drod5_0.dat` | `assets/GatEB` |

```
./build_macos.sh --steam-variant gateb --universal --steam-sdk /path/to/steamworks-sdk
./build_macos.sh --steam-variant tss   --universal        # (--steam is shorthand for tss)
```

Both build with `-DSTEAMBUILD` and link `libsteam_api`; `tss` adds `-DSTEAMBUILD_TSS_APP`.
The variant table (AppIDs etc.) lives near the top of `build_macos.sh`. Details:
- `--steam-sdk` points at an unpacked Steamworks SDK (default `~/Developer/steam-sdk`);
  its `public/steam` headers are added to the include path.
- `libsteam_api.dylib` is taken from the SDK's `redistributable_bin/osx/` (override with
  `--steam-lib`), staged into `deps/lib`, vendored into the bundle, and signed.
- Game data comes from the per-app dir `Master/Darwin/localdata/<App>/` (`TSS`/`GatEB`,
  gitignored) by default; override with `--dat-dir`. The app icon comes from the committed
  `Master/Darwin/assets/<App>/` (its `DROD.icns`, plus any icon bitmaps overlaid onto the
  shared drodData ones). Switching variants cleans the shared `steam/` build dir
  automatically, so no manual clean is needed.
- A Steam build is normally launched through the Steam client (which supplies the app id),
  so the shipped bundle contains no `steam_appid.txt`. To run **outside** Steam for local
  testing, add `--steam-appid`: it writes the variant's AppID to
  `Contents/MacOS/steam_appid.txt` in the bundle. **Never upload that file to the depot** —
  omit `--steam-appid` for a clean shippable app.
- Just the binary: `make drod-steam STEAM_SDK=/path/... [STEAM_TSS=1]` (STEAM_TSS=1 = TSS).

## Standalone older games (KDD / JtRH / TCB / GatEB)

The same engine also builds the older games as **standalone** (non-Steam) apps,
selected with `--game`. Each is gated behind its own compile-time define and
looks for its own data file:

| `--game` | game | define | data file | app / icon / data folder |
|----------|------|--------|-----------|--------------------------|
| `kdd`   | King Dugan's Dungeon        | `-DKDD_STANDALONE`   | `drod-kdd5_0.dat`   | `KDD`   |
| `jtrh`  | Journey to Rooted Hold      | `-DJTRH_STANDALONE`  | `drod-jtrh5_0.dat`  | `JtRH`  |
| `tcb`   | The City Beneath            | `-DTCB_STANDALONE`   | `drod-tcb5_0.dat`   | `TCB`   |
| `gateb` | Gunthro and the Epic Blunder| `-DGATEB_STANDALONE` | `drod-gateb5_0.dat` | `GatEB` |

```
./build_macos.sh --game kdd            # -> "DROD KDD.app"
./build_macos.sh --game tcb --universal
```

- `--game` cannot be combined with `--steam` (these are standalone builds).
- App name defaults to `DROD <App>` (override with `--name`); the icon comes from
  `Master/Darwin/assets/<App>/DROD.icns` (falls back to the TSS icon with a warning
  if not present yet), and branded `Icon*.bmp` from the same folder.
- Game data comes from `Master/Darwin/localdata/<App>/` (gitignored) by default.
  The build copies **only this game's** data set — `drod-<game>5_0.dat` plus its
  `drod-<game>5_0_NN.dat` add-on packs — so a folder shared with another game's
  dats (e.g. Steam GatEB's `drod5_0*.dat` alongside standalone GatEB's
  `drod-gateb5_0*.dat` in `localdata/GatEB/`) never leaks mismatched data into the
  bundle.
- Switching game/variant cleans the reused `custom/` (or `release`/`debug`) build
  dir automatically, since make can't see the changed `-D` flag.
- Just the binary: `make drod-custom GAME=kdd` (or `jtrh`/`tcb`/`gateb`).

## Demo build (TSS)

```
./build_macos.sh --demo        # -> "DROD TSS Demo.app"
```

There is **no demo compile flag** — a TSS demo is the *plain* TSS binary (data
`drod5_0.dat`) shipping a demo hold. The engine decides demo-vs-full at runtime in
`CDrodScreen::IsGameFullVersion()`: a build is a demo when its installed hold has
≤50 level entrances (`EntrancesInFullVersion()`), which enables the "Buy Now" menu
item and the sell/buy screens.

`--demo` is purely a convenience: it brands the app `DROD TSS Demo` and sources the
demo `drod5_0.dat` from `Master/Darwin/localdata/TSS-Demo/` (gitignored), keeping it
apart from the Steam TSS data in `localdata/TSS/` (`drod-tss5_0.dat`). It is the
same binary as a full standalone TSS, so it shares the `custom/` build objects — no
recompile when switching between them. TSS-standalone only (not with `--game` /
`--steam`).
