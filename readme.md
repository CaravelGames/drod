# DROD: The Second Sky
**(c) Caravel Games 2005-2017**

----------------------------

### Building the application

This is the DROD:TSS source.
It may be used under the terms of the MPL and other licenses, described in the Licenses subdirectory.

##### Building for Windows

Currently there are two documents that describe the build process:
1. *CompilingDrod_Win.md* - which uses automated script for building the libraries and is aimed at VS2013.
2. *CompilingDrod_Win_Old.md* - which was assembled for DROD TCB that should work for older versions of Visual Studio

Windows, Linux and Mac OS X compilations are supported.
Workspace files are included for Visual Studio 2005, Visual Studio 2008 and Visual Studio 2013 in the Master subdirectory.
Workspace files for Microsoft Visual Studio 6.0 and Visual Studio 2002 are also present but likely outdated.

##### Linux builds

scons and makefile files for Linux are included in Master/Linux.

cd to Master/Linux and run 'scons -h' to see build options.
Options are passed to scons as 'option=value' (without the quotes), separated by spaces.
For example, if you wanted to build DROD with FMOD audio for amd64/x86-64 (which wouldn't work since FMOD 3.x doesn't exist for amd64, but hey, let's ignore such trifling details), you would do:

`scons audio=fmod arch=amd64`

The dist option should be left at the default (none).


##### Mac builds

Makefiles for Mac are included in Master/Darwin.

To build a 64-bit binary, run "make custom" (etc.)

### Contributing

1. Visit [the bugs board](http://forum.caravelgames.com/viewboard.php?BoardID=7) on the forum and select a bug you'd like to fix. 
2. If the way to fix the bug is clear or clearly defined in the thread, make a reply that you'll fix it. 
3. If the way to fix is not clear please first discuss with one of the devs how to approach it.
4. After you fix the bug make a pull request to the main repository and it will be reviewed.
5. If the PR passes the code review we'll ask you to provide a diff file, we'll patch the private repos with the changes and publish a new version of the public codebase here in this repository.

**Please note** that if you're making any changes that modify or fix the logic of the game we'll need you to provide tests in the Test project to ensure things are working as intended.

### To use SDL_mixer instead of FMOD:

Add `-lSDL_mixe`r to the link flags (LDFLAGS_* in `Master/Linux/Confi`g) and either (1) add `-DUSE_SDL_MIXER` flag to the C++ flags (i.e., CXXFLAGS_* in Config) -- this will require a clean recompile, since the dependency system doesn't detect command-line changes yet; or (2) add `#define USE_SDL_MIXER` to the top of `FrontEndLib/Sound.h` -- this will make the dependency system pick up the change, so a normal recompile is sufficient.

### Including content media

A fresh Data/drod5_0.dat file is provided, but you will need to use the drod5_0.dat file provided in an official Caravel installation of DROD (demo or full) to make this compiled source run.  Note that files obtained from a Caravel installation are protected under the DROD copyright and are not for distribution, public or private.

Graphics/styles:
Stub and mod versions of the tile and styles graphics are available, and can be found on the Development Board on http://forum.caravelgames.com Add these graphics files to the Data/Bitmaps directory for in-game use. To avoid running in fullscreen mode, run the application with the "nofullscreen" command line parameter.

Music:
 - The music engine supports sampled sound formats (e.g. Ogg Vorbis, wave, mp3).
 - Add music files to Data/Music for in-game use.
 - Modify the [Music] section of Data/drod.ini to apply your selection of music files.
 - To avoid running with any sound or music,
 - run the application with the "nosound" command line parameter.
