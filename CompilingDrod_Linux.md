# Building DROD

1. Install Docker.
2. Run the script `./Scripts/Linux/build.sh all`
3. This will build a release version of DROD in `./Master/Linux/builds/custom.release.x86_64/drod` and DROD RPG in `./drodrpg/Master/Linux/builds/custom.release.x86_64/drod`
4. Copy appropriate `drod5_0.dat` files
5. Copy `libSDL2-2.0.so.0` into the game directory for redistribution

# Building Steam DROD

1. Steam SDK needs to be unpacked into Master/Linux/steam
2. `./Scripts/Linux/build.sh docker-up`
3. `./Scripts/Linux/build.sh build-deps`
4. `./Scripts/Linux/build.sh docker-bar` — this will open bash in the docker container in which:
    1. `cd Master/Linux`
    2. `./ninjamaker -steam -64` — Generate script for compiling DROD:GATEB
    3. `./ninjamaker -steam-tss -64` — Generate script for compiling DROD:TSS
    4. `ninja -f build.steam.release.x86_64.ninja` — Builds DROD:GATEB for Steam
    4. `ninja -f build.steam-tss.release.x86_64.ninja` — Builds DROD:TSS for Steam

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

# CLion on Linux
Below are steps and tips to get the project working correctly using Docker builds with full debugging support in CLion on Linux.

### Custom Build Target
1. Open _Settings &rarr; Custom Build Targets_.
2. Create a new target, call it **"Custom Debug Build"**.
3. Under **Build** click on "&hellip;" and add a new **External Tool**
   1. Name the new tool **"Custom Debug Build (Docker)"**.
   2. In **Program to Run** put `Scripts/Linux/build.sh`
   3. In **Arguments** put `build-drod --mode=custom --debug`
   4. In **Working Directory** put `Scripts/Linux/`
4. Select **"Custom Debug Build (Docker)"** as the build command for the tool.

### Build configuration
1. Open _Menu &rarr; Run &rarr; Edit configurations..._.
2. Add a new configuration of type **Custom Build Application** and call it **"Custom Debug"**.
3. For **Target** select **Custom Debug Build**.
4. In **Executable** put `Master/Linux/builds/custom.debug.x86_64/drod` (the file won't exist unless you have already build the game once).
5. In **Working Directory** put `Master/Linux/builds/caravel.debug.x86_64`

### Code completion
For code completion to work you need to create `CMakeLists.txt` in the root directory:

```cmake
cmake_minimum_required(VERSION 3.20)
project(DrodCPP)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 11)

# Point to your compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dummy target so CLion is happy
file(GLOB_RECURSE ALL_SOURCES "*.cpp" "*.h")
add_executable(dummy_for_clion ${ALL_SOURCES})
target_compile_options(dummy_for_clion PRIVATE -x c++)
target_compile_options(dummy_for_clion PRIVATE -std=c++11)
target_include_directories(dummy_for_clion PRIVATE
        ./
        ./FrontEndLib
        ./BackEndLib
        ./DROD
        ./DRODLib
        ./DRODLibTests
        ./CaravelNet
        ./metakit/include
)
```

### Breakpoints and debugging
In order for path mapping to work correctly in CLion create file `~/.gdbinit` (in your home directory) with the following contents:

```text
set substitute-path /drod <ABSOLUTE PATH TO YOUR PROJECT FOLDER>
```

Update `<ABSOLUTE PATH TO YOUR PROJECT FOLDER>` to point to your project folder, its root specifically, no trailing slash.

### Finally
You should now be able to build DROD and debug it in CLion using its native build capabilities.