# Compiling DROD in 2020

In this document you will find a list of extra steps I had to take to compile DROD on OS X 10.15 in 2020. This is an addition to the instructions listed in the Config file, which are still valid. This does not include altering the Makefile/Config to work with your specific setup, but it should help you fix most of the problems that occur.

* Install g++ version 4.9 through Homebrew, and point CC and CXX in the Makefile to that version
* Get gcc v3 pod_char_traits.h from the internet
* Remove -DCARAVELBUILD compiler flag due to missing CaravelNet source
* Instead of relying on the `install_name_tool` commands in the Makefile I used macdylibbundler to bundle dylibs correctly (https://github.com/auriamg/macdylibbundler) 

Library versions used:
* Metakit 2.4.9.7
* SDL2 2.0.12
* SDL2_mixer 2.0.4
* SDL2_TTF 2.0.13 (2.0.15 did not work for me, as the included FreeType version needs a version of libpng that is incompatible with the version used in DROD)
* FreeType 2.4.12 (same version as used in SDL_TTF)
* Expat 2.0.1
* libpng 1.2.12
* libjpeg 6b-16
* zlib 1.2.12
* libogg 1.1.3
* libvorbis 1.1.2
* libtheora 1.0alpha7
* jsoncpp 0.6.0-rc2