# Building DROD

1. Download and build the libraries.
2. Update `include` and `library` paths in the project files.
3. Build the project.
4. Copy DLLs.
5. Copy `drod5_0.dat`.

#### Short version

There is a Python script that downloads and builds all of the required libraries in `Scripts/InstallDependencies.win32.vs2013.py`, optimized for Visual Studio 2013 but it should work for newer versions with little changes. In order to build with this script do the following:

1. Edit `Scripts/InstallDependencies.win32.vs2013.py` and provide the correct path to `DevEnv.com`.
2. Run the script.
3. After it finishes, open the solution `Master/Master.2013.sln` and build the project - the paths are all set to work with the dependency installing script.
4. After the script finished building copy DLLs from `Deps/Dll/Debug` or `Deps/Dll/Release` and paste them to `DROD/DebugVS2013/` or `DROD/Release/` respectively.
5. Go to your installation of DROD (either Demo or Full version) and copy `Data/drod5_0.dat` and paste it to `DROD/DebugVS2013/Data/` or `DROD/Release/Data/`.
6. Run the game.

#### Download and build the libraries

The game requires the following libraries. Different versions may potentially be used but these are the ones present in the automatic dependency installation script:

 - `curl-7.27.0` => https://curl.haxx.se/download/curl-7.27.0.zip
 - `expat-2.1.0` => https://downloads.sourceforge.net/project/expat/expat/2.1.0/expat-2.1.0.tar.gz
 - `fmodapi-375-win` => http://cdn.retrocade.net/fmodapi375win.zip (This file is no longer readily available from FMOD website)
 - `jpeg-6b` => http://cdn.retrocade.net/jpeg-6b.zip (This package contains libraries prebuilt in VS2013, original sources may be used and built manually)
 -  `json-0.6.2-rc2` => http://cdn.retrocade.net/jsoncpp-src-0.6.0-rc2.zip (This is exactly the same as the official version with the single exception being that assembly generation was removed from the release target in VS project)
 -  `libogg-1.3.0` => http://downloads.xiph.org/releases/ogg/libogg-1.3.0.tar.gz
 -  `libtheora-1.1.1` => http://downloads.xiph.org/releases/theora/libtheora-1.1.1.tar.gz
 -  `libvorbis-1.3.3` => http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.5.tar.gz
 -  `lpng-1512` => https://downloads.sourceforge.net/project/libpng/libpng15/older-releases/1.5.12/lpng1512.zip
 -  `metakit-2.4.9.5` => https://github.com/jcw/metakit/archive/2.4.9.5.tar.gz
 -  `sdl2-2.0.5` => https://www.libsdl.org/release/SDL2-2.0.5.zip
 -  `sdl-ttf-2.20.1` => https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.20.1/SDL2_ttf-devel-2.20.1-VC.zip
 -  `zlib` => http://www.zlib.net/fossils/zlib-1.2.11.tar.gz

You need the DLLs from the following libraries:
 - fmod
 - curl
 - expat
 - libfreetype (from sdl-ttf)
 - SDL2
 - SDL2_ttf
 - zlib (might best to use the one from lpng)

##### Update include and library paths in the project files

There are two ways to approach this - either copy the necessary include and library files from their original projects to a new location (like the build script does) and only have to add a single path to each projects.
Alternatively you can add all the necessary path to the projects or to your global settings ([more information here](https://www.curlybrace.com/words/2012/12/17/setting-global-c-include-paths-in-visual-studio-2012-and-2011-and-2010/)).

##### Build the project

Now you can open one of the solutions located in `Master/` depending on which project files you've modified and build the project like you'd always do. It's highly likely the first time you'll encounter some errors - maybe you built a library incorrectly, maybe you have to update include/library paths, maybe you need to change some setting specific to your development environment.

##### Copy DLLs

Once the project is built copy the necessary DLL files to `DROD/Release` or `DROD/DebugVS2013` depending on which version was built.

#### Copy drod5_0.dat

Go to your installation of DROD (either Demo or Full version) and copy `Data/drod5_0.dat` and paste it to `DROD/DebugVS2013/Data/` or `DROD/Release/Data/`.

After all that you should be able to run the game without a problem! If it worked, then congratulations!

## Troubleshooting

Here is a list of all the problems that have been reported to happen while building the game and ways to work around them.

###### json_value.asm: No such file or directory

This may occur when building a release version of JSON library using the official sources - open the project in Visual Studio, go to `libjson` -> Properties -> C/C++ -> Output Files and set *Assembler Output* to *No Listing*.

##### Procedure entry point InterlockedCompareExchange@12 could not be located in the dynamic link library SDL2_ttf.dll

This is a runtime error caused by missing or incorrect version of `libfreetype-6.dll`. Make sure you have the DLL in your DROD directory and use the version taken from `SDL_ttf` library.
