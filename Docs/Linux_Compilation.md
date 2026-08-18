# How to build DROD on Linux

DROD official builds are currently made using Docker in a _standardized_ environment. We will be using `Scripts/Linux/build.sh` for making the builds. Internally the compilation is powered by Ninja and our `ninjamaker` shell script.

## First build

1. [Install Docker](https://docs.docker.com/engine/install/).
2. Run `Scripts/Linux/build.sh all` &mdash; this:
   - Builds the docker container and starts it.
   - Downloads and compiles the dependencies.
   - Builds a release build of DROD in `Master/Linux/builds/custom.release.x86_64/drod`.
   - Builds a release build of DROD Tests in `Master/Linux/builds/custom.release.x86_64/drod_tests`.
   - Builds a release build of DROD RPG in `drodrpg/Master/Linux/builds/custom.release.x86_64/drod`.
   - Copies the required shared libraries into each directory
3. Copy `Data/` directory from your installation of DROD and DROD RPG to the appropriate directories above.

## Subsequent builds
Run `Scripts/Linux/build.sh build-drod` to build DROD again. Use `--debug` if you want a debug build and `--mode=<MODE>` if you want a different mode; keep in mind both of those changes affect the path.

You can also choose to use VSCode ([instructions](Linux_VSCodeDevelopment.md)) or CLion ([instructions](Linux_CLionDevelopment.md)) for development.

## Steam build

1. Download the latest version of Steam SDK and unpack it into `Master/Linux/steam`.
2. Make sure docker is up and dependencies were build.
2. Run `Scripts/Linux/build.sh --mode=steam` &mdash; create Steam GatEB build.
3. Run `Scripts/Linux/build.sh --mode=steam-tss` &mdash; create Steam TSS build.