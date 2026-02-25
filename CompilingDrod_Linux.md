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
