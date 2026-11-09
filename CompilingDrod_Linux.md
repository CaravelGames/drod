# Building DROD

See [Docs/Linux_Compilation.md](./Docs/Linux_Compilation.md) for details on how to build DROD on Linux.

# Building new releases
This is to whoever is tasked with making the proper, official builds. Step by step, all things:

```bash
# Copy the official `CaravelNetInterface.cpp` and `CaravelNetInterface.h`
# into the project

# Assumed the deps are already built

cd Scripts/Linux

# Run and enter docker, that's where we will make the builds for consistency
./build.sh docker-up
./build.sh docker-bash

# We are inside docker container now

# Go to the builds directory
cd Master/Linux

# Making caravel build
./ninjamaker -64 -caravel
ninja -f build.caravel.release.x86_64.ninja

# Making Steam TSS Build
./ninjamaker -64 -steam-tss
ninja -f build.steam-tss.release.x86_64.ninja

# Making Steam GatEB Build
./ninjamaker -64 -steam
ninja -f build.steam.release.x86_64.ninja
```

## Caravel build (Full version)
1. Take the existing build of the game and unpack it
2. Replace assets:
    - `Data/Bitmaps` — replace the files with a new version (one by one) from the private Data repo
    - `Data/Fonts` — replace the files with a new version from the private Data repo
    - `Data/Help` — delete the directory `1/` and copy over `Data/Help/1` from the public repo
    - `Data/Homemade` — do not modify
    - `Data/Licenses` — replace contents with `Licenses/` from the public repo
    - `Data/drod5_0.dat` — replace with the latest release of the dat file
3. Replace binary `Bin/linux-x86_64/drod-tss` with the compiled file
4. Replace SDL2 shared library in `Bin/linux-x86_64/` with the one found on the docker container at path `/usr/lib/x86_64-linux-gnu/libSDL2-2.0.so.0`. Just copy the file to be in the same directory as the game binary
5. Archive into a .tar.gz archive

## Caravel build (Demo)
1. Take the above package
2. Replace the full version dat file with the demo version dat file
3. Archive into a .tar.gz archive

## Steam build (TSS)
1. Take the existing DROD package from Steam
2. Replace assets:
    - `Data/Bitmaps` — replace the files with a new version (one by one) from the private Data repo
    - `Data/Fonts` — replace the files with a new version from the private Data repo
    - `Data/Help` — delete the directory `1/` and copy over `Data/Help/1` from the public repo
    - `Data/Licenses` — replace contents with `Licenses/` from the public repo
    - `Data/*.dat` — Remove all `.dat` files from `Data/`
3. Replace binary `Bin/linux-x86_64` with the compiled file from `Master/Linux/builds/steam-tss.release.x86_64/drod`
4. Replace SDL2 shared library in `Bin/linux-x86_64/` with the one found on the docker container at path `/usr/lib/x86_64-linux-gnu/libSDL2-2.0.so.0`. Just copy the file to be in the same directory as the game binary
5. Replace steamlib shared library in `Bin/linux-x86_64/libsteam_api.so` with the one from the steam SDK used to compile the game, by default would be in `Master/Linux/steam/redistributable_bin/linux64/libsteam_api.so`

## Steam build (GatEB)
1. Take the above package
2. Replace `steam_appid.txt` with the one from the Gunthro's local files on Steam (VERY IMPORTANT!)
3. Replace binary `Bin/linux-x86_64` with the compiled file from `Master/Linux/builds/steam.release.x86_64/drod`

## Old readme

Below is archived information about Linux builds from Readme.

Ninja generator and build files for Linux are included in Master/Linux. cd to Master/Linux and run './ninjamaker' then './build' or './build -clean' for clean rebuild.

When debugging build issues edit the build file from 'ninja -k 0' to 'ninja -k N' so ninja stops building after N jobs fail.

>Options are passed to scons as 'option=value' (without the quotes), separated by spaces.
>For example, if you wanted to build DROD with FMOD audio for amd64/x86-64 (which wouldn't work since >FMOD 3.x doesn't exist for amd64, but hey, let's ignore such trifling details), you would do:
>
>`scons audio=fmod arch=amd64`
>
>The dist option should be left at the default (none).

It is possible to build DROD on a Raspberry Pi 4 (or probably 5), although this will require increasing the amount of swap space available for lower memory units (e.g. less than 4Gb). Failure to provide sufficient swap space will result in the Pi crashing. Increased swap space is not required to run DROD once built.

To build, run `./ninjamaker -arch aarch64 -no-static` followed by `./build` in the `Master/Linux` directory. You will need to have built and installed Metakit before doing this, else it will fail when linking.

