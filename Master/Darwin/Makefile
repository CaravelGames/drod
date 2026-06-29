# DROD macOS Makefile, by Gerry Jo Jellestad; adapted for OS X by Weston Markham.
# Modernized for Apple Clang + Homebrew (arm64 / universal). See Config and
# build_macos.sh. The legacy Intel/PPC/gcc-4.9 path has been removed.

MAKEFLAGS += --no-print-directory

default: help
all: drod-custom drodutil-custom
cus custom drod drod-cus: drod-custom
drodutil drodutil-cus: drodutil-custom
rel release drod-rel: drod-release
dbg debug drod-dbg: drod-debug
steam: drod-steam
drodutil-rel: drodutil-release
drodutil-dbg: drodutil-debug

include Config

ALLBTYPES = custom release debug steam

ALLMODS = BackEndLib DRODLib FrontEndLib CaravelNet DROD DRODUtil
DRODMOD = BackEndLib DRODLib FrontEndLib CaravelNet DROD
UTILMOD = BackEndLib DRODLib FrontEndLib DROD DRODUtil

# Generate all possible ALLMODS-ALLBTYPES combinations
ALLMODS_ALL = $(shell for btype in $(ALLBTYPES); do \
	for mod in $(ALLMODS); do $(ECHO) $$mod-$$btype; done; done)

ALLMODS_CLEAN     = $(ALLMODS:%=clean-%)
ALLMODS_CLEAN_ALL = $(ALLMODS_ALL:%=clean-%)

.PHONY: all custom rel release debug steam \
	drod drod-custom drod-rel drod-release drod-debug drod-steam \
	drodutil drodutil-custom drodutil-rel drodutil-release drodutil-debug drodutil-steam \
	$(ALLMODS) $(ALLMODS_ALL) $(ALLMODS_CLEAN) $(ALLMODS_CLEAN_ALL) \
	$(ALLBTYPES:%=clean-%) clean link help

# Build output directories (relative to Master/Darwin).
OBJDIR = obj
LIBDIR = lib
BINDIR = bin

EXCLUDE_DRODUtil = DrodBitmapManager.cpp EditRoomWidget.cpp EvilEyeGazeEffect.cpp FaceWidget.cpp \
	MapWidget.cpp PendingPlotEffect.cpp RoomEffectList.cpp RoomWidget.cpp StrikeOrbEffect.cpp \
	TileImageCalcs.cpp

TOBJDIR = $(BTYPE)/$(OBJDIR)
TLIBDIR = $(BTYPE)/$(LIBDIR)
TBINDIR = $(BTYPE)/$(BINDIR)
ifneq ($(MODULE),)
	MODSDIR = $(SRCDIR)/$(MODULE)
	MODODIR = $(TOBJDIR)/$(MODULE)
	# Source lists come from the maintained <Module>.2019.vcxproj files
	SOURCES_FROM_DSP = \
		$(shell $(GREP) 'ClCompile Include=.*\.cpp' $(MODSDIR)/$(MODULE).2019.vcxproj |\
			$(SED) 's,.*Include="\([^"]*\)".*,$(MODSDIR)/\1,')
	ifneq ($(EXCLUDE_$(MODULE)),)
		SOURCES = $(filter-out $(EXCLUDE_$(MODULE):%=$(MODSDIR)/%), \
			$(SOURCES_FROM_DSP))
	else
		SOURCES = $(SOURCES_FROM_DSP)
	endif
	OBJECTS = $(SOURCES:$(MODSDIR)/%.cpp=$(MODODIR)/%.o) $(EXTRA_OBJ_$(MODULE):%.o=$(MODODIR)/%.o)
	DEPFILES = $(OBJECTS:%.o=%.dep)
	# On macOS's case-insensitive filesystem, putting the module dir on the
	# angle-include (-I) path makes system headers like <wchar.h> wrongly resolve
	# to BackEndLib/Wchar.h. Use -iquote for the module dir (searched only for ""
	# includes); cross-module <BackEndLib/...> includes still resolve via -I$(SRCDIR).
	CPPFLAGS = $(CPPFLAGS_$(MODULE)) -iquote$(MODSDIR) -I$(SRCDIR)
endif

DRODLIBS = $(TLIBDIR)/DROD.a $(TLIBDIR)/FrontEndLib.a \
	$(TLIBDIR)/DRODLib.a $(TLIBDIR)/BackEndLib.a $(TLIBDIR)/CaravelNet.a
UTILLIBS = $(TLIBDIR)/DRODUtil.a $(TLIBDIR)/DROD.a $(TLIBDIR)/FrontEndLib.a $(TLIBDIR)/DRODLib.a $(TLIBDIR)/BackEndLib.a

help:
	@echo "DROD macOS build. Edit Config if needed, then run 'make <TARGET>[-<TYPE>]':"
	@echo
	@echo "  drod         Build and link the drod executable."
	@echo "  drodutil     Build and link the drodutil executable."
	@echo "  all          Build both."
	@echo "  <Module>     Build a single module (BackEndLib, DRODLib, FrontEndLib,"
	@echo "               CaravelNet, DROD, DRODUtil)."
	@echo
	@echo "<TYPE> is one of: custom (default), release, debug."
	@echo "Pass UNIVERSAL_BUILD=1 for a universal (x86_64 + arm64) binary."
	@echo "Clean targets: 'make clean[-<TARGET>][-<TYPE>]'."
	@echo
	@echo "To produce a self-contained .app/.dmg, use ./build_macos.sh instead."

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

$(MODODIR)/%.o: $(MODSDIR)/%.cpp
	@$(ECHO) "    DepComp $<"
	$(CXXDEP) $(CXXFLAGS_common) $(CXXFLAGS) $(CPPFLAGS) -c $< $(DEPOUT) $(@:%.o=%.tmp) -o $@
	@$(SED) "s,\($(MODODIR)/\)*\(.*\).o:,$(MODODIR)/\2.o:," < $(@:%.o=%.tmp) > $(@:%.o=%.dep)
	@$(RM_F) $(@:%.o=%.tmp)

$(TLIBDIR)/$(MODULE).a: $(OBJECTS)
	$(if $(MODULE),, $(error Don't call this target directly))
	@$(ECHO) "    Archive $@"
	@$(RM_F) $@
	@$(AR) $@ $(OBJECTS)
	@$(RANLIB) $@
	@$(ECHO) " "

link: $(MODS)
	$(if $(LINKTARGET),, $(error Don't call this target directly))
	@$(MAKE) $(LINKTARGET)
	@$(ECHO) "    Done!"
	@$(ECHO) " "

$(TBINDIR)/$(DROD) $(TBINDIR)/$(DRODUTIL): $($(XTYPE)LIBS)
	@$(ECHO) "    Linking $@:"
	$(LD) $($(XTYPE)LIBS) $(LDFLAGS_$(XTYPE)_$(BTYPE)) -o $@

ifneq ($(DEPFILES),)
ifneq ($(wildcard $(MODODIR)/*.dep),)
include $(wildcard $(MODODIR)/*.dep)
endif
endif
