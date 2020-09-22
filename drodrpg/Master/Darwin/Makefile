# DROD Makefile, by Gerry Jo Jellestad; adapted for OS X by Weston Markham

DIRECTORY_VERSION=1.2.7
#DIRECTORY_VERSION=1.2.7-demo
OFFICIAL_NAME=DROD-RPG
ifeq ($(ARCHITECTURE), x86_64)
BUNDLE_NAME=$(OFFICIAL_NAME)-64bit.app
else
BUNDLE_NAME=$(OFFICIAL_NAME)-32bit.app
endif

MAKEFLAGS += --no-print-directory

default: help
all: all-cus
all-cus: drod-custom drodutil-custom
all-rel all-release: drod-release drodutil-release
all-dbg all-debug: drod-debug drodutil-debug
all-prof all-profile: drod-profile drodutil-profile
cus custom drod drod-cus: drod-custom
drodutil drodutil-cus: drodutil-custom
rel release drod-rel: drod-release
drodutil-rel: drodutil-release
dbg debug drod-dbg: drod-debug
prof profile drod-prof: drod-profile
drodutil-dbg: drodutil-debug
drodutil-prof: drodutil-profile
beta: drod-beta
install install-cus: install-custom
install-rel: install-release
install-dbg: install-debug
install-prof: install-profile

include Config

INST_HELP_LANG = 1

ALLBTYPES = custom release debug profile beta

ALLMODS = BackEndLib DRODLib FrontEndLib CaravelNet DROD DRODUtil
DRODMOD = BackEndLib DRODLib FrontEndLib CaravelNet DROD
#ALLMODS = BackEndLib DRODLib FrontEndLib DROD DRODUtil
#DRODMOD = BackEndLib DRODLib FrontEndLib DROD
UTILMOD = BackEndLib DRODLib FrontEndLib DROD DRODUtil

# Generate all possible ALLMODS-ALLBTYPES combinations
ALLMODS_ALL = $(shell for btype in $(ALLBTYPES); do \
	for mod in $(ALLMODS); do $(ECHO) $$mod-$$btype; done; done)

ALLMODS_CLEAN     = $(ALLMODS:%=clean-%)
ALLMODS_CLEAN_ALL = $(ALLMODS_ALL:%=clean-%)

.PHONY: all all-rel all-release all-debug all-prof all-profile \
	clean custom rel release debug prof profile beta \
	drod drod-custom drod-rel drod-release drod-debug \
	drod-prof drod-profile drod-beta \
	drodutil drodutil-custom drodutil-rel drodutil-release \
	drodutil-dbg drodutil-debug \
	drodutil-prof drodutil-profile install $(ALLBTYPES:%=install-%) \
	$(ALLMODS) $(ALLMODS_ALL) $(ALLMODS_CLEAN) $(ALLMODS_CLEAN_ALL) \
	$(ALLBTYPES:%=clean-%) uninstall link

# These must be absolute
OBJDIR = obj
LIBDIR = lib
BINDIR = bin

#EXCLUDE_BackEndLib =
#EXCLUDE_FrontEndLib =
EXCLUDE_DRODUtil = DrodBitmapManager.cpp EditRoomWidget.cpp EvilEyeGazeEffect.cpp FaceWidget.cpp \
	MapWidget.cpp PendingPlotEffect.cpp RoomEffectList.cpp RoomWidget.cpp StrikeOrbEffect.cpp \
	TileImageCalcs.cpp # problem with parsing .dsp?
#EXCLUDE_DROD =
ifdef PPC_BUILD
EXTRA_OBJ_DROD = SDLMain.o
endif

TOBJDIR = $(BTYPE)/$(OBJDIR)
TLIBDIR = $(BTYPE)/$(LIBDIR)
TBINDIR = $(BTYPE)/$(BINDIR)
ifneq ($(MODULE),)
	MODSDIR = $(SRCDIR)/$(MODULE)
	MODODIR = $(TOBJDIR)/$(MODULE)
	SOURCES_FROM_DSP = \
		$(shell $(GREP) "SOURCE=.*\.cpp" $(MODSDIR)/$(MODULE).dsp |\
			$(SED) "s,SOURCE=.\\\\,$(MODSDIR)/," |\
			$(SED) "s,SOURCE=..\\\\.*\\\\,$(MODSDIR)/,")
	ifneq ($(EXCLUDE_$(MODULE)),)
		SOURCES = $(filter-out $(EXCLUDE_$(MODULE):%=$(MODSDIR)/%), \
			$(SOURCES_FROM_DSP))
	else
		SOURCES = $(SOURCES_FROM_DSP)
	endif
	OBJECTS = $(SOURCES:$(MODSDIR)/%.cpp=$(MODODIR)/%.o) $(EXTRA_OBJ_$(MODULE):%.o=$(MODODIR)/%.o)
	DEPFILES = $(OBJECTS:%.o=%.dep)
#	CPPFLAGS = -I$(MODSDIR) -I$(SRCDIR) $(CPPFLAGS_$(MODULE))
	CPPFLAGS = $(CPPFLAGS_$(MODULE)) -I$(MODSDIR) -I$(SRCDIR)
endif

ifdef PPC_BUILD
DRODLIBS = $(TOBJDIR)/DROD/SDLMain.o $(TLIBDIR)/DROD.a $(TLIBDIR)/FrontEndLib.a \
	$(TLIBDIR)/DRODLib.a $(TLIBDIR)/BackEndLib.a $(TLIBDIR)/CaravelNet.a
else
DRODLIBS = $(TLIBDIR)/DROD.a $(TLIBDIR)/FrontEndLib.a \
	$(TLIBDIR)/DRODLib.a $(TLIBDIR)/BackEndLib.a $(TLIBDIR)/CaravelNet.a
endif
UTILLIBS = $(TLIBDIR)/DRODUtil.a $(TLIBDIR)/DROD.a $(TLIBDIR)/FrontEndLib.a $(TLIBDIR)/DRODLib.a $(TLIBDIR)/BackEndLib.a 

help:
	@echo "Help for the DROD build system: First edit the Config file, and follow its"
	@echo "instructions. Then run 'make <TARGET>[-<TYPE>]', where <TARGET> is:"
	@echo
	@echo "  drod         Build and link the drod executable."
	@echo "  drodutil     Build and link the drodutil executable."
	@echo "  all          Build and link all of the above."
	@echo "  BackEndLib   Only build the BackEndLib module."
	@echo "  FrontEndLib  Only build the FrontEndLib module."
	@echo "  DRODLib      Only build the DRODLib module."
	@echo "  DROD         Only build the DROD module (note the big case)."
	@echo "  bundle       Build a bundle for Mac OS X."
	@echo "  install      Install the specified build type.  (not maintained)"
	@echo
	@echo "<TYPE> can be one of (aliases are only available for some targets):"
	@echo
	@echo "  custom (cus)    Custom build: If in doubt, use this one [default]."
	@echo "  release (rel)   Release build: The binary distro was built with this."
	@echo "  debug (dbg)     Debug build: Enables debug information."
	@echo "  profile (prof)  Profile build: Enables profiling."
	@echo
	@echo "If <TARGET> is 'drod', only <TYPE> has to be specified."
	@echo "At least one of the two needs to be present."
	@echo "There's also clean targets available: 'make clean[-<TARGET>][-<TYPE>]'."
	@echo
	@echo "Good luck building DROD!"

$(ALLBTYPES:%=drod-%):
	@$(MAKE) link LINKTARGET="$(@:drod-%=%)/$(BINDIR)/$(DROD)" \
		MODS="$(DRODMOD:%=%$(@:drod%=%))" XTYPE="DROD" BTYPE=$(@:drod-%=%)
$(ALLBTYPES:%=drodutil-%):
	@$(MAKE) link LINKTARGET="$(@:drodutil-%=%)/$(BINDIR)/$(DRODUTIL)" \
		MODS="$(UTILMOD:%=%$(@:drodutil%=%))" XTYPE="UTIL" BTYPE=$(@:drodutil-%=%)
clean:
	$(if $(MODULE), \
		($(if $(BTYPE), \
			$(RM_RF) $(DEPFILE) $(MODODIR) $(TLIBDIR)/$(MODULE).a, \
			$(MAKE) clean-$(MODULE)) ), \
		$(if $(BTYPE), \
			$(RM_RF) $(BTYPE), \
			$(RM_RF) $(ALLBTYPES) ) )

$(ALLMODS_ALL):
	@$(if $(BTYPE), (\
	 $(ECHO) "    Preparing to build $(@:%-$(BTYPE)=%).a, $(BTYPE) build"; \
	 $(ECHO) "    CXXFLAGS=$(CXXFLAGS_common) $(CXXFLAGS_$(BTYPE))"; \
	 $(MKDIR) $(BTYPE) $(TOBJDIR) $(TOBJDIR)/$(@:%-$(BTYPE)=%) $(TLIBDIR) $(TBINDIR); \
	 $(MAKE) $(BTYPE)/$(LIBDIR)/$(@:%-$(BTYPE)=%).a \
		MODULE="$(@:%-$(BTYPE)=%)" CXXFLAGS="$(CXXFLAGS_$(BTYPE))" \
	), \
	 $(MAKE) $@ BTYPE=$(filter $(ALLBTYPES), $(subst -, ,$@)) \
	)

$(ALLMODS_CLEAN_ALL):
	@$(if $(BTYPE), (\
	 $(MAKE) clean MODULE=$(@:clean-%-$(BTYPE)=%) \
	), \
	 $(MAKE) $@ BTYPE=$(filter $(ALLBTYPES), $(subst -, ,$@)) \
	)

$(ALLMODS_CLEAN):
	@$(MAKE) MODULE="$(@:clean-%=%)" BTYPE="*" clean

$(ALLBTYPES:%=clean-%):
	@$(MAKE) clean BTYPE=$(@:clean-%=%)

$(ALLBTYPES:%=bundle-%):
	@$(MAKE) MODULE=drod BUNDLE_EXECUTABLE_NAME=drod BTYPE=$(@:bundle-%=%) bundle

$(ALLBTYPES:%=demobundle-%):
	@$(MAKE) MODULE=drod BUNDLE_EXECUTABLE_NAME=drod-demo BTYPE=$(@:demobundle-%=%) DMG_EXT=-demo bundle

#TODO:  clean targets need to clean out the bundle

#indented libs below should be statically linked
EXPAT_DYL=libexpat.1.dylib
#FMOD_DYL=fmod_cfm.shlb
  FREETYPE_DYL=libfreetype.6.dylib
  JPEG_DYL=libjpeg.62.dylib
METAKIT_DYL=libmk4.dylib
  OGG_DYL=libogg.0.dylib
  PNG_DYL=libpng.dylib
SDL_DYL=libSDL-1.2.0.dylib
SDL_TTF_DYL=libSDL_ttf-2.0.0.dylib
SDL_MIXER_DYL=libSDL_mixer-1.2.0.dylib
  VORBIS_DYL=libvorbis.0.dylib
  VORBISFILE_DYL=libvorbisfile.3.dylib
CURL_DYL=libcurl.4.dylib
ZLIB_DYL=libz.1.dylib
JSON_DYL=libjson_linux-gcc-4.2.1_libmt.dylib

CONTENTS_DIR=$(BUNDLE_NAME)/Contents
FRAMEWORKS_DIR=$(CONTENTS_DIR)/Frameworks
REL_FRAMEWORKS_DIR=@executable_path/../Frameworks
RESOURCES_DIR=$(CONTENTS_DIR)/Resources
MACOS_DIR=$(CONTENTS_DIR)/MacOS
DYLIB_DIR=$(LIB_LIB_DIR)

GAMENAME=drod
APP_DIR=$(TBINDIR)/$(GAMENAME)-$(DIRECTORY_VERSION)$(DMG_EXT)
FULL_FRAMEWORKS_DIR=$(APP_DIR)/$(FRAMEWORKS_DIR)
FULL_RESOURCES_DIR=$(APP_DIR)/$(RESOURCES_DIR)
FULL_EXE_DIR=$(APP_DIR)/$(MACOS_DIR)/$(BUNDLE_EXECUTABLE_NAME)

bundle: $(TBINDIR)/$(GAMENAME)
	$(MKDIR) $(APP_DIR)
	-chmod -R a+w $(APP_DIR)/*
	rm -rf $(APP_DIR)/*
#	-chmod -R a+w $(APP_DIR)/$(BUNDLE_NAME)
#	rm -rf $(APP_DIR)/$(BUNDLE_NAME)
	$(MKDIR) $(APP_DIR)/$(CONTENTS_DIR)
	$(MKDIR) $(APP_DIR)/$(MACOS_DIR)
	sed 's/%EXECUTABLE_NAME%/'$(BUNDLE_EXECUTABLE_NAME)'/;s/%VERSION%/'$(DIRECTORY_VERSION)'/' Info.plist.template > $(APP_DIR)/$(CONTENTS_DIR)/Info.plist
	cp $< $(APP_DIR)/$(MACOS_DIR)/$(BUNDLE_EXECUTABLE_NAME)

	$(MKDIR)                                       $(FULL_FRAMEWORKS_DIR)
	cp $(EXPAT_ROOT)/lib/$(EXPAT_DYL)              $(FULL_FRAMEWORKS_DIR)
##	cp $(FMOD_ROOT)/api/lib/$(FMOD_DYL)            $(FULL_FRAMEWORKS_DIR)
	cp $(FREETYPE_ROOT)/lib/$(FREETYPE_DYL)        $(FULL_FRAMEWORKS_DIR)
#	cp $(JPEG_ROOT)/lib/$(JPEG_DYL)                $(FULL_FRAMEWORKS_DIR)
	cp $(METAKIT_ROOT)/lib/$(METAKIT_DYL)          $(FULL_FRAMEWORKS_DIR)
	cp $(LIBOGG_ROOT)/lib/$(OGG_DYL)               $(FULL_FRAMEWORKS_DIR)
#	cp $(PNG_ROOT)/$(PNG_DYL)                      $(FULL_FRAMEWORKS_DIR)
	cp $(SDL_ROOT)/lib/$(SDL_DYL)                  $(FULL_FRAMEWORKS_DIR)
	cp $(SDL_TTF_ROOT)/lib/$(SDL_TTF_DYL)          $(FULL_FRAMEWORKS_DIR)
	cp $(SDL_MIXER_ROOT)/lib/$(SDL_MIXER_DYL)      $(FULL_FRAMEWORKS_DIR)
	cp $(LIBVORBIS_ROOT)/lib/$(VORBIS_DYL)         $(FULL_FRAMEWORKS_DIR)
	cp $(LIBVORBIS_ROOT)/lib/$(VORBISFILE_DYL)     $(FULL_FRAMEWORKS_DIR)
	cp $(CURL_ROOT)/lib/$(CURL_DYL)                $(FULL_FRAMEWORKS_DIR)
	cp $(ZLIB_ROOT)/lib/$(ZLIB_DYL)                $(FULL_FRAMEWORKS_DIR)
	cp $(JSON_ROOT)/lib/$(JSON_DYL)                $(FULL_FRAMEWORKS_DIR)

	-strip -x $(FULL_FRAMEWORKS_DIR)/$(EXPAT_DYL)
#	-strip -x $(FULL_FRAMEWORKS_DIR)/$(FMOD_DYL)
	-strip -x $(FULL_FRAMEWORKS_DIR)/$(FREETYPE_DYL)
#	-strip -x $(FULL_FRAMEWORKS_DIR)/$(JPEG_DYL)
	-strip -x $(FULL_FRAMEWORKS_DIR)/$(METAKIT_DYL)
	-strip -x $(FULL_FRAMEWORKS_DIR)/$(OGG_DYL)
#	-strip -x $(FULL_FRAMEWORKS_DIR)/$(PNG_DYL)
	-strip -x $(FULL_FRAMEWORKS_DIR)/$(SDL_DYL)
	-strip -x $(FULL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)
	-strip -x $(FULL_FRAMEWORKS_DIR)/$(SDL_MIXER_DYL)
	-strip -x $(FULL_FRAMEWORKS_DIR)/$(VORBIS_DYL)
	-strip -x $(FULL_FRAMEWORKS_DIR)/$(VORBISFILE_DYL)
	-strip -x $(FULL_FRAMEWORKS_DIR)/$(CURL_DYL)
	-strip -x $(FULL_FRAMEWORKS_DIR)/$(ZLIB_DYL)
	-strip -x $(FULL_FRAMEWORKS_DIR)/$(JSON_DYL)

#Commented out ones are being statically linked:
#	install_name_tool -change $(JPEG_ROOT)/lib/$(JPEG_DYL)            $(REL_FRAMEWORKS_DIR)/$(JPEG_DYL)       $(FULL_EXE_DIR)
	install_name_tool -change $(METAKIT_ROOT)/lib/$(METAKIT_DYL)      $(REL_FRAMEWORKS_DIR)/$(METAKIT_DYL)    $(FULL_EXE_DIR)
#	install_name_tool -change $(PNG_ROOT)/lib/$(PNG_DYL)              $(REL_FRAMEWORKS_DIR)/$(PNG_DYL)        $(FULL_EXE_DIR)
	install_name_tool -change $(EXPAT_ROOT)/lib/$(EXPAT_DYL)          $(REL_FRAMEWORKS_DIR)/$(EXPAT_DYL)      $(FULL_EXE_DIR)
#	install_name_tool -change $(FMOD_ROOT)/lib/$(FMOD_DYL)            $(REL_FRAMEWORKS_DIR)/$(FMOD_DYL)       $(FULL_EXE_DIR)
	install_name_tool -change $(SDL_ROOT)/lib/$(SDL_DYL)              $(REL_FRAMEWORKS_DIR)/$(SDL_DYL)        $(FULL_EXE_DIR)
	install_name_tool -change $(SDL_TTF_ROOT)/lib/$(SDL_TTF_DYL)      $(REL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)    $(FULL_EXE_DIR)
	install_name_tool -change $(SDL_MIXER_ROOT)/lib/$(SDL_MIXER_DYL)  $(REL_FRAMEWORKS_DIR)/$(SDL_MIXER_DYL)  $(FULL_EXE_DIR)
	install_name_tool -change $(LIBOGG_ROOT)/lib/$(OGG_DYL)           $(REL_FRAMEWORKS_DIR)/$(OGG_DYL)        $(FULL_EXE_DIR)
	install_name_tool -change $(LIBVORBIS_ROOT)/lib/$(VORBIS_DYL)     $(REL_FRAMEWORKS_DIR)/$(VORBIS_DYL)     $(FULL_EXE_DIR)
	install_name_tool -change $(LIBVORBIS_ROOT)/lib/$(VORBISFILE_DYL) $(REL_FRAMEWORKS_DIR)/$(VORBISFILE_DYL) $(FULL_EXE_DIR)
	install_name_tool -change $(CURL_ROOT)/lib/$(CURL_DYL)            $(REL_FRAMEWORKS_DIR)/$(CURL_DYL)       $(FULL_EXE_DIR)
	install_name_tool -change $(ZLIB_ROOT)/lib/$(ZLIB_DYL)            $(REL_FRAMEWORKS_DIR)/$(ZLIB_DYL)       $(FULL_EXE_DIR)
#Alternate location
	install_name_tool -change /usr/lib/$(ZLIB_DYL)              $(REL_FRAMEWORKS_DIR)/$(ZLIB_DYL)       $(FULL_EXE_DIR)
	install_name_tool -change $(JSON_ROOT)/lib/$(JSON_DYL)            $(REL_FRAMEWORKS_DIR)/$(JSON_DYL)       $(FULL_EXE_DIR)
#Alternate location
	install_name_tool -change buildscons/linux-gcc-4.2.1/src/lib_json/$(JSON_DYL)            $(REL_FRAMEWORKS_DIR)/$(JSON_DYL)       $(FULL_EXE_DIR)

	install_name_tool -change $(METAKIT_DYL)                          $(REL_FRAMEWORKS_DIR)/$(METAKIT_DYL)    $(FULL_EXE_DIR)

	install_name_tool -change $(FREETYPE_ROOT)/lib/$(FREETYPE_DYL)    $(REL_FRAMEWORKS_DIR)/$(FREETYPE_DYL)   $(FULL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)
	install_name_tool -change $(SDL_ROOT)/lib/$(SDL_DYL)              $(REL_FRAMEWORKS_DIR)/$(SDL_DYL)        $(FULL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)
	install_name_tool -change $(ZLIB_ROOT)/lib/$(ZLIB_DYL)            $(REL_FRAMEWORKS_DIR)/$(ZLIB_DYL)       $(FULL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)
#	install_name_tool -change $(JPEG_ROOT)/lib/$(JPEG_DYL)            $(REL_FRAMEWORKS_DIR)/$(JPEG_DYL)       $(FULL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)
	install_name_tool -change $(LIBVORBIS_ROOT)/lib/$(VORBISFILE_DYL) $(REL_FRAMEWORKS_DIR)/$(VORBISFILE_DYL) $(FULL_FRAMEWORKS_DIR)/$(SDL_MIXER_DYL)
	install_name_tool -change $(LIBVORBIS_ROOT)/lib/$(VORBIS_DYL)     $(REL_FRAMEWORKS_DIR)/$(VORBIS_DYL)     $(FULL_FRAMEWORKS_DIR)/$(SDL_MIXER_DYL)
	install_name_tool -change $(LIBOGG_ROOT)/lib/$(OGG_DYL)           $(REL_FRAMEWORKS_DIR)/$(OGG_DYL)        $(FULL_FRAMEWORKS_DIR)/$(SDL_MIXER_DYL)
	install_name_tool -change $(SDL_ROOT)/lib/$(SDL_DYL)              $(REL_FRAMEWORKS_DIR)/$(SDL_DYL)        $(FULL_FRAMEWORKS_DIR)/$(SDL_MIXER_DYL)
	install_name_tool -change $(LIBVORBIS_ROOT)/lib/$(VORBIS_DYL)     $(REL_FRAMEWORKS_DIR)/$(VORBIS_DYL)     $(FULL_FRAMEWORKS_DIR)/$(VORBISFILE_DYL)
	install_name_tool -change $(LIBOGG_ROOT)/lib/$(OGG_DYL)           $(REL_FRAMEWORKS_DIR)/$(OGG_DYL)        $(FULL_FRAMEWORKS_DIR)/$(VORBISFILE_DYL)
	install_name_tool -change $(LIBOGG_ROOT)/lib/$(OGG_DYL)           $(REL_FRAMEWORKS_DIR)/$(OGG_DYL)        $(FULL_FRAMEWORKS_DIR)/$(VORBIS_DYL)
#Alternate original location for referenced dynamic libraries:
	install_name_tool -change $(DYLIB_DIR)/$(EXPAT_DYL)         $(REL_FRAMEWORKS_DIR)/$(EXPAT_DYL)      $(FULL_EXE_DIR)
#	install_name_tool -change $(DYLIB_DIR)/$(FMOD_DYL)          $(REL_FRAMEWORKS_DIR)/$(FMOD_DYL)       $(FULL_EXE_DIR)
	install_name_tool -change $(DYLIB_DIR)/$(SDL_DYL)           $(REL_FRAMEWORKS_DIR)/$(SDL_DYL)        $(FULL_EXE_DIR)
	install_name_tool -change $(DYLIB_DIR)/$(SDL_TTF_DYL)       $(REL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)    $(FULL_EXE_DIR)
	install_name_tool -change $(DYLIB_DIR)/$(SDL_MIXER_DYL)     $(REL_FRAMEWORKS_DIR)/$(SDL_MIXER_DYL)  $(FULL_EXE_DIR)
	install_name_tool -change $(DYLIB_DIR)/$(CURL_DYL)          $(REL_FRAMEWORKS_DIR)/$(CURL_DYL)       $(FULL_EXE_DIR)
	install_name_tool -change $(DYLIB_DIR)/$(ZLIB_DYL)          $(REL_FRAMEWORKS_DIR)/$(ZLIB_DYL)       $(FULL_EXE_DIR)

	install_name_tool -change $(DYLIB_DIR)/$(SDL_DYL)           $(REL_FRAMEWORKS_DIR)/$(SDL_DYL)        $(FULL_FRAMEWORKS_DIR)/$(SDL_DYL)
	install_name_tool -change $(DYLIB_DIR)/$(SDL_TTF_DYL)       $(REL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)    $(FULL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)

#	install_name_tool -change $(DYLIB_DIR)/$(FREETYPE_DYL)      $(REL_FRAMEWORKS_DIR)/$(FREETYPE_DYL)   $(FULL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)
	install_name_tool -change $(DYLIB_DIR)/$(SDL_DYL)           $(REL_FRAMEWORKS_DIR)/$(SDL_DYL)        $(FULL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)
#	install_name_tool -change $(DYLIB_DIR)/$(JPEG_DYL)          $(REL_FRAMEWORKS_DIR)/$(JPEG_DYL)       $(FULL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)
#	install_name_tool -change $(DYLIB_DIR)/$(VORBISFILE_DYL)    $(REL_FRAMEWORKS_DIR)/$(VORBISFILE_DYL) $(FULL_FRAMEWORKS_DIR)/$(SDL_MIXER_DYL)
	install_name_tool -change $(DYLIB_DIR)/$(VORBIS_DYL)        $(REL_FRAMEWORKS_DIR)/$(VORBIS_DYL)     $(FULL_FRAMEWORKS_DIR)/$(SDL_MIXER_DYL)
	install_name_tool -change $(DYLIB_DIR)/$(OGG_DYL)           $(REL_FRAMEWORKS_DIR)/$(OGG_DYL)        $(FULL_FRAMEWORKS_DIR)/$(SDL_MIXER_DYL)
#	install_name_tool -change $(DYLIB_DIR)/$(SDL_DYL)           $(REL_FRAMEWORKS_DIR)/$(SDL_DYL)        $(FULL_FRAMEWORKS_DIR)/$(SDL_MIXER_DYL)
	install_name_tool -change $(DYLIB_DIR)/$(VORBIS_DYL)        $(REL_FRAMEWORKS_DIR)/$(VORBIS_DYL)     $(FULL_FRAMEWORKS_DIR)/$(VORBISFILE_DYL)
	install_name_tool -change $(DYLIB_DIR)/$(OGG_DYL)           $(REL_FRAMEWORKS_DIR)/$(OGG_DYL)        $(FULL_FRAMEWORKS_DIR)/$(VORBISFILE_DYL)
	install_name_tool -change $(DYLIB_DIR)/$(OGG_DYL)           $(REL_FRAMEWORKS_DIR)/$(OGG_DYL)        $(FULL_FRAMEWORKS_DIR)/$(VORBIS_DYL)
#Another alternate location for referenced dynamic libraries:
#	install_name_tool -change /sw/lib/$(SDL_DYL)                $(REL_FRAMEWORKS_DIR)/$(SDL_DYL)        $(FULL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)
	install_name_tool -change /usr/lib/$(ZLIB_DYL)              $(REL_FRAMEWORKS_DIR)/$(ZLIB_DYL)       $(FULL_FRAMEWORKS_DIR)/$(CURL_DYL)
	install_name_tool -change /usr/lib/$(ZLIB_DYL)              $(REL_FRAMEWORKS_DIR)/$(ZLIB_DYL)       $(FULL_FRAMEWORKS_DIR)/$(FREETYPE_DYL)
	install_name_tool -change /usr/lib/$(ZLIB_DYL)              $(REL_FRAMEWORKS_DIR)/$(ZLIB_DYL)       $(FULL_FRAMEWORKS_DIR)/$(SDL_TTF_DYL)


	$(MKDIR) $(APP_DIR)/$(RESOURCES_DIR)
	cp DROD.icns $(APP_DIR)/$(RESOURCES_DIR)
	$(UNRAR_X) $(DATA_ARCHIVE_$(BTYPE)) $(APP_DIR)/$(RESOURCES_DIR)
	# (note that DATA_ARCHIVE_beta should always point to DemoData.rar)
	-(cd $(FULL_RESOURCES_DIR); mv DemoData Data)
	-(cd $(FULL_RESOURCES_DIR); mv FullData Data)
	cp $(SRCDIR)/Data/Bitmaps/Cursor16.png    $(FULL_RESOURCES_DIR)/Data/Bitmaps/Cursor16x16.png
	cp $(SRCDIR)/Data/Bitmaps/hourglass16.png $(FULL_RESOURCES_DIR)/Data/Bitmaps/hourglass16x16.png
	cp $(SRCDIR)/Data/Bitmaps/internet16.png  $(FULL_RESOURCES_DIR)/Data/Bitmaps/internet16x16.png
#	$(MOGRIFY) $(SRCDIR)/DRODSetup/BeethroIcon128x128.png $(FULL_RESOURCES_DIR)/Data/Bitmaps/Icon128x128.png
# (doesn't work)

	cp $(FULL_RESOURCES_DIR)/Data/Licenses/EULA.txt $(APP_DIR)
	cp $(SRCDIR)/DRODSetup/changelog.txt $(APP_DIR)

	chmod -R a-w $(APP_DIR)/$(BUNDLE_NAME)/Contents
#	-mv $(APP_DIR)/$(BUNDLE_NAME) $(APP_DIR)/$(BUNDLE_NAME)

#Make the bundle.  Rename the build directory to the official release name of the app before packaging.
#Remove any existing bundle before attempting this.
#	-chmod -R a+w $(TBINDIR)/$(OFFICIAL_NAME)-$(DIRECTORY_VERSION)
#	rm -rf $(TBINDIR)/$(OFFICIAL_NAME)-$(DIRECTORY_VERSION)
#	mv $(TBINDIR)/$(GAMENAME)-$(DIRECTORY_VERSION) $(TBINDIR)/$(OFFICIAL_NAME)-$(DIRECTORY_VERSION)
	hdiutil create -srcfolder $(APP_DIR) $(APP_DIR).dmg -ov

$(ALLBTYPES:%=install-%):
	$(if $(wildcard $(@:install-%=%)/$(BINDIR)/*),, \
		$(error No built targets for "$(@:install-%=%)" build type))
	$(if $(INSTALL_TYPE),, $(error INSTALL_TYPE not set))
	$(if $(BIN_$(INSTALL_TYPE)),, $(error BIN path not set for "$(INSTALL_TYPE)"))
	$(if $(RES_$(INSTALL_TYPE)),, $(error RES path not set for "$(INSTALL_TYPE)"))
	$(if $(DAT_$(INSTALL_TYPE)),, $(error DAT path not set for "$(INSTALL_TYPE)"))
	@$(ECHO) "Installing from \"$(@:install-%=%)\""
	$(INSTALL) $(INST_DAT_DIRS) -d $(DESTDIR)/$(DAT_$(INSTALL_TYPE))
	$(INSTALL) $(INST_RES_DIRS) -d $(DESTDIR)/$(BIN_$(INSTALL_TYPE))
	$(INSTALL) $(INST_RES_DIRS) -d \
		$(DESTDIR)/$(RES_$(INSTALL_TYPE))/Bitmaps \
		$(DESTDIR)/$(RES_$(INSTALL_TYPE))/Fonts \
		$(DESTDIR)/$(RES_$(INSTALL_TYPE))/Music \
		$(DESTDIR)/$(RES_$(INSTALL_TYPE))/Sounds \
		$(INST_HELP_LANG:%=$(DESTDIR)/$(RES_$(INSTALL_TYPE))/Help/%/images)
	$(INSTALL) $(INST_BIN_DIRS) $(@:install-%=%)/$(BINDIR)/* \
		$(DESTDIR)/$(BIN_$(INSTALL_TYPE))
	$(INSTALL) $(INST_RES_FILES) $(SRC_RES_BMP:%=$(SRC_DATADIR)/Bitmaps/%) \
		$(DESTDIR)/$(RES_$(INSTALL_TYPE))/Bitmaps
	$(INSTALL) $(INST_RES_FILES) $(SRC_RES_FONT:%=$(SRC_DATADIR)/Fonts/%) \
		$(DESTDIR)/$(RES_$(INSTALL_TYPE))/Fonts
	$(INSTALL) $(INST_RES_FILES) $(SRC_RES_MSX:%=$(SRC_DATADIR)/Music/%) \
		$(DESTDIR)/$(RES_$(INSTALL_TYPE))/Music
	$(INSTALL) $(INST_RES_FILES) $(SRC_RES_SFX:%=$(SRC_DATADIR)/Sounds/%) \
		$(DESTDIR)/$(RES_$(INSTALL_TYPE))/Sounds
	for lang in $(INST_HELP_LANG); do \
		$(INSTALL) $(INST_RES_FILES) \
			$(SRC_RES_HELP:%=$(SRC_DATADIR)/Help/$$lang/%) \
			$(DESTDIR)/$(RES_$(INSTALL_TYPE))/Help/$$lang; \
		$(INSTALL) $(INST_RES_FILES) \
			$(SRC_RES_HIMG:%=$(SRC_DATADIR)/Help/$$lang/images/%) \
			$(DESTDIR)/$(RES_$(INSTALL_TYPE))/Help/$$lang/images; done
	$(INSTALL) $(INST_DAT_FILES) \
		$(SRC_DAT:%=$(SRC_DATADIR)/%) $(DESTDIR)/$(DAT_$(INSTALL_TYPE))
	@echo "Install done!"

‰uninstall:
	$(if $(INSTALL_TYPE),, $(error INSTALL_TYPE not set))
	$(if $(BIN_$(INSTALL_TYPE)),, $(error BIN path not set for "$(INSTALL_TYPE)"))
	$(if $(RES_$(INSTALL_TYPE)),, $(error RES path not set for "$(INSTALL_TYPE)"))
	$(if $(DAT_$(INSTALL_TYPE)),, $(error DAT path not set for "$(INSTALL_TYPE)"))
	$(RM_RF) $(DESTDIR)/$(BIN_$(INSTALL_TYPE))/$(DROD) \
		$(DESTDIR)/$(BIN_$(INSTALL_TYPE))/$(DRODUTIL) \
		$(DESTDIR)/$(RES_$(INSTALL_TYPE)) \
		$(DESTDIR)/$(DAT_$(INSTALL_TYPE))

$(MODODIR)/%.o: $(MODSDIR)/%.cpp
	@$(ECHO) "    DepComp $<"
	$(CXXDEP) $(CXXFLAGS_common) $(CXXFLAGS) $(CPPFLAGS) -c $< $(DEPOUT) $(@:%.o=%.tmp) -o $@
	@$(SED) "s,\($(MODODIR)/\)*\(.*\).o:,$(MODODIR)/\2.o:," < $(@:%.o=%.tmp) > $(@:%.o=%.dep)
	@$(RM_F) $(@:%.o=%.tmp)

$(MODODIR)/%.o: $(SRCDIR)/Master/Darwin/%.m
	$(CXX) $(CXXFLAGS) $(SDL_INCLUDE) -c $< -o $@

$(TLIBDIR)/$(MODULE).a: $(OBJECTS)
	$(if $(MODULE),, $(error Don't call this target directly))
	@$(ECHO) "    Archive $@ ($(SOURCE2))"
	@$(AR) $@ $(OBJECTS)
	@$(RANLIB) $@
	@$(ECHO) " "

link: $(MODS)
	$(if $(LINKTARGET),, $(error Don't call this target directly))
	@$(MAKE) $(LINKTARGET)
	@$(ECHO) "    Done!"
	@$(ECHO) " "

$(TBINDIR)/$(DROD) $(TBINDIR)/$(DRODUTIL) $(TBINDIR)/$(DRODCGI): $($(XTYPE)LIBS)
	@$(ECHO) "    Linking $@:"
	$(LD) $($(XTYPE)LIBS) $(LDFLAGS_$(XTYPE)_$(BTYPE)) -o $@

ifneq ($(DEPFILES),)
ifneq ($(wildcard $(MODODIR)/*.dep),)
include $(wildcard $(MODODIR)/*.dep)
endif
endif

