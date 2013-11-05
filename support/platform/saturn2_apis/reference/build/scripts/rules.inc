
###############################################
###############################################
##
##	This file is included at the
##		end of a makefile
##
##	Mike S. Brown:	25/03/2004
##
###############################################
###############################################


###############################################
## Define XGEN_TARGETDIR
## Create GEN_TARGETDIR
##
ifneq (,$(findstring mtx,$(IMG_CPU)))
 CPU_MTX=true
endif

ifneq (,$(findstring meta,$(IMG_CPU)))
 CPU_META=true
endif

ifneq (,$(findstring mtp,$(IMG_CPU)))
 CPU_META=true
endif

UNAME := $(shell uname)

###############################################
## Define XGEN_TARGETDIR
## Create GEN_TARGETDIR
##
ifdef GEN_TARGETDIR
  ifneq (,$(findstring CYGWIN_NT,$(UNAME)))
    export XGEN_TARGETDIR := $(shell cygpath -au $(GEN_TARGETDIR))
  else
    export XGEN_TARGETDIR := $(shell pwd)/$(GEN_TARGETDIR)
  endif
  ifeq (,$(XGEN_TARGETDIR))
    $(error Bad GEN_TARGETDIR path: "$(GEN_TARGETDIR)" )
  endif
  ZXT := $(shell if [ ! -d $(GEN_TARGETDIR) ]; then mkdir -p $(GEN_TARGETDIR); fi; )
  ##$(warning **Creating: $(GEN_TARGETDIR)** )
endif

##################################################
## Pull in appropriate rules to build RPC client
##
ifneq (,$(GEN_CLIENTSRC)$(GEN_SERVERSRC))
	ifneq (,$(findstring CYGWIN,$(UNAME)))
		TOOLSDIR=$(IMG_WORKROOT)/bin/tools/win32
		XGEN_TMP=`cygpath -u $(IMG_WORKROOT)/rpc/$(IMG_PROJ)/include/$(subst .h,_rpc.h,$(<F))`
	else
		TOOLSDIR=$(IMG_WORKROOT)/bin/tools/linux
		XGEN_TMP=$(IMG_WORKROOT)/rpc/$(IMG_PROJ)/include/$(subst .h,_rpc.h,$(<F))
	endif
#$(warning **XGEN_TMP: $(XGEN_TMP)** )
	ZXT := $(shell if [ ! -d $(XGEN_TMP) ]; then mkdir -p $(XGEN_TMP); fi; )
endif

##################################################
## Pull in appropriate rules to build RPC client
##
ifneq (,$(GEN_CLIENTSRC))
	RPC_INC_FILE=
	RPC_OVERRIDE_STRING = $(shell echo '$(GEN_CLIENTSRC:.h=)_RPC_MODE' | tr a-z A-Z)
	ifneq (,$($(RPC_OVERRIDE_STRING)))
		RPC_INC_FILE=$(IMG_WORKROOT)/build/scripts/rpc_rules_$($(RPC_OVERRIDE_STRING)).inc
    $(warning Overriding project default RPC type for library '$(GEN_CLIENTSRC)' - now using RPC type '$(RPC_INC_FILE)')
	else
		ifeq (,$(PROJ_DEFAULT_RPC_MODE))
    $(error In order to remote APIs, a PROJ_DEFAULT_RPC_MODE must be specified)
		else
			RPC_INC_FILE=$(IMG_WORKROOT)/build/scripts/rpc_rules_$(PROJ_DEFAULT_RPC_MODE).inc
		endif
	endif

  $(if $(wildcard $(RPC_INC_FILE)),,$(error Unable to find RPC rules file '$(RPC_INC_FILE)'))

	include $(RPC_INC_FILE)
endif

##################################################
## Pull in appropriate rules to build RPC server
##
ifneq (,$(GEN_SERVERSRC))
	RPC_INC_FILE=
	RPC_OVERRIDE_STRING = $(shell echo '$(GEN_SERVERSRC:.h=)_RPC_MODE' | tr a-z A-Z)
	ifneq (,$($(RPC_OVERRIDE_STRING)))
		RPC_INC_FILE=$(IMG_WORKROOT)/build/scripts/rpc_rules_$($(RPC_OVERRIDE_STRING)).inc
    $(warning Overriding project default RPC type for library '$(GEN_SERVERSRC)' - now using RPC type '$(RPC_INC_FILE)')
	else
		ifeq (,$(PROJ_DEFAULT_RPC_MODE))
    $(error In order to remote APIs, a PROJ_DEFAULT_RPC_MODE must be specified)
		else
			RPC_INC_FILE=$(IMG_WORKROOT)/build/scripts/rpc_rules_$(PROJ_DEFAULT_RPC_MODE).inc
		endif
	endif

  $(if $(wildcard $(RPC_INC_FILE)),,$(error Unable to find RPC rules file '$(RPC_INC_FILE)'))

	include $(RPC_INC_FILE)
endif

###############################################
## Define XBUILDDIR
## Define XTARGETS
## Create BUILDDIR
##
ifeq ($(LIB_BUILD),true)
	ifeq ($(LIB_NEW),true)
		export LIB_NEW=false
		TMP_BUILDDIR=$(LIB_BUILDDIR)
		XTARGETS=$(LIB_TARGET)
	endif
else
	TMP_BUILDDIR=$(BUILDDIR)
	XTARGETS=$(TARGETS)
endif

ifdef TMP_BUILDDIR
	ifneq (,$(findstring CYGWIN_NT,$(UNAME)))
		export XBUILDDIR := $(shell cygpath -au $(TMP_BUILDDIR))
		export BUILDDIRABS := $(XBUILDDIR)
	else
#		export XBUILDDIR=$(shell pwd)/$(TMP_BUILDDIR)
		export XBUILDDIR=$(TMP_BUILDDIR)
		export BUILDDIRABS=$(TMP_BUILDDIR)
	endif
	ifeq (,$(BUILDDIRABS))
		$(error Bad BUILDDIR path: "$(TMP_BUILDDIR)" )
	endif
	ZXT := $(shell if [ ! -d $(TMP_BUILDDIR) ]; then mkdir -p $(TMP_BUILDDIR); fi; )
	##$(warning **Creating: $(TMP_BUILDDIR)** )
endif

###############################################################################
## If the XTARGETS contains a ".a" (library) then create XLIBBUILDDIR which
## is the relative path for the build output directory from the "build"
## directory containing the top-level makefile for the library.
## NOTE: The relatiev path to the ".a" is used to setup the ".a" as second
## target fro "AR"
##
ifneq (,$(filter %.a,$(XTARGETS)))
	TMP1=$(patsubst %/build,%,$(CURDIR))
	XLIBBUILDDIR := $(subst $(TMP1)/,../,$(abspath $(XBUILDDIR)))/
#$(warning **TMP1: $(TMP1)** )
#$(warning **CURDIR: $(CURDIR)** )
#$(warning **XBUILDDIR: $(XBUILDDIR)** )
#$(warning **(abspath XBUILDDIR): $(abspath $(XBUILDDIR))** )
#$(warning **XLIBBUILDDIR: $(XLIBBUILDDIR)** )
#$(warning **BUILDDIR: $(BUILDDIR)** )
endif

###############################################
## Create $(OBJECTS) from $(SRC1) (Assembler source that comes first in the link)
##
ifneq (,$(SRC1))
#	OBJECTS += $(addprefix $(XBUILDDIR)/,$(SRC1:.s=.o))
	OBJECTS += $(addprefix $(XBUILDDIR)/,$(subst .s,.o,$(notdir $(SRC1))))
    CVPATH=$(SRCDIR) $(subst $(SRCDIR)./,,$(addprefix $(SRCDIR),$(dir $(SRC1))))
    vpath %.s $(CVPATH)
else
	SRC1=
endif

###############################################
## Create $(OBJECTS) from $(SRC) "C" files
##
ifneq (,$(SRC))
#	OBJECTS += $(addprefix $(XBUILDDIR)/,$(SRC:.c=.o))
    OBJECTS += $(addprefix $(XBUILDDIR)/,$(subst .c,.o,$(notdir $(SRC))))
    CVPATH=$(SRCDIR) $(subst $(SRCDIR)./,,$(addprefix $(SRCDIR),$(dir $(SRC))))
    vpath %.c $(CVPATH)
#$(warning **vpath $(CVPATH)** )
else
	SRC=
endif

###############################################
## Create $(OBJECTS) from $(SRCCPP) "CPP" files
##
ifneq (,$(SRCCPP))
#	OBJECTS += $(addprefix $(XBUILDDIR)/,$($(notdir $(SRCCPP)):.cpp=.o))
	OBJECTS += $(addprefix $(XBUILDDIR)/,$(subst .cpp,.o,$(notdir $(SRCCPP))))
    CVPATH=$(SRCDIR) $(subst $(SRCDIR)./,,$(addprefix $(SRCDIR),$(dir $(SRCCPP))))
    vpath %.cpp $(CVPATH)
#(warning **vpath $(CVPATH)** )
else
	SRCCPP=
endif

###############################################
## Create $(OBJECTS) from $(SRCASM) Assembler files
##
ifneq (,$(SRCASM))
#	OBJECTS += $(addprefix $(XBUILDDIR)/,$(SRCASM:.s=.o))
	OBJECTS += $(addprefix $(XBUILDDIR)/,$(subst .s,.o,$(notdir $(SRCASM))))
    CVPATH=$(SRCDIR) $(subst $(SRCDIR)./,,$(addprefix $(SRCDIR),$(dir $(SRCASM))))
    vpath %.s $(CVPATH)
else
	SRCASM=
endif

###############################################
## Add source dirs to include list
##
ifdef SRCDIR
	INCLUDES += -I$(SRCDIR)
endif
ifdef SRCDIR1
	INCLUDES += -I$(SRCDIR1)
endif


###############################################
###############################################
###############################################
## Default rule
##	This rule is executed if no other
##	rule is specified.
module: $(GENSRC_TARGETS) $(OBJECTS) $(GEN_OBJECTS) $(XTARGETS)


###############################################
## Filter $(XTARGETS) to find any target libs
##
TARGETLIB = $(addprefix $(XLIBBUILDDIR),$(filter %.a,$(XTARGETS)))

ifneq (,$(TARGETLIB))
	export THIS_LIB=$(XBUILDDIR)/$(subst .a,.ax,$(filter %.a,$(XTARGETS)))
$(TARGETLIB): $(OBJECTS) $(GEN_OBJECTS) $(SUBLIBS)
endif
#$(warning **THIS_LIB: $(THIS_LIB)** )

###############################################
## Include auto-dependancies for libraries
## if not cleaning
## this works 'cos libs on cmdline doesn't
## get passed beyond thread
ifneq ($(MAKECMDGOALS),clean)
 ifneq ($(MAKECMDGOALS),libclean)
   ifdef LIBS
    -include $(LIBS:%.a=%.axdd)
   endif
 endif
endif

###############################################
## Sub-make rules
##
ifneq (NONE,$(LIBS))
#$(LIBS): FORCE
ifeq ($(MAKECMDGOALS),libclean)
$(LIBS): FORCE
else
$(LIBS):
endif
	@echo ==================================
	@echo Thread: $(THREAD) - Library: $(@F)
	$(shell if [ ! -d $(@D) ]; then mkdir -p $(@D); fi; ) \
	if ([ ! -e $@ ] || [ -w $@ ]); then \
		export LIB_BUILD=true; \
		export LIB_NEW=true; \
		export LIB_BUILDDIR=$(@D); \
		export LIB_TARGET=$(@F); \
		rm -f  $(@); \
		rm -f  $(subst .a,.add,$(@)); rm -f  $(subst .a,.addd,$(@));\
		if ([ -d $(@D)/../../build ]); then \
			echo Makefile: $(abspath $(@D)/../../build/makefile); \
			$(MAKE) $(SILENT) -C $(@D)/../../build $(filter-out libs,$(MAKECMDGOALS)); \
		elif ([ -d $(@D)/../../../build ]); then \
			echo Makefile: $(abspath $(@D)/../../../build/makefile); \
			$(MAKE) $(SILENT) -C $(@D)/../../../build $(filter-out libs,$(MAKECMDGOALS)); \
		elif ([ -d $(@D)/../../../../build ]); then \
			echo Makefile: $(abspath $(@D)/../../../../build/makefile); \
			$(MAKE) $(SILENT) -C $(@D)/../../../../build $(filter-out libs,$(MAKECMDGOALS)); \
		else \
			echo ERROR: Cannot find build dir for: $(@D); \
			exit -1 ; \
		fi&& \
		export LIB_BUILD=false; \
	else \
		echo .........Read only; \
	fi
else
## Dummy target for (LIBS == NONE)
$(LIBS): ;
endif

## general purpose SUBLIBS rule
## used for build & clean
##	$(MAKE) $(SILENT) -C $(SUBPATH)$@/build $(MAKECMDGOALS)
ifneq (,$(SUBLIBS))
$(SUBLIBS): FORCE
	$(MAKE) $(SILENT) -C $(SUBPATH)$@/build $(MAKECMDGOALS)
endif


ifeq ($(filter libs,$(MAKECMDGOALS)),libs)
  G_SUBMODS_GOALS=
endif
ifeq ($(filter libclean,$(MAKECMDGOALS)),libclean)
  G_SUBMODS_GOALS=clean clobber
endif

ifneq (,$(G_SUBMODS))
ifeq ($(GMAKEON), 1)
$(G_SUBMODS): FORCE
	$(MAKE) -C $@/gmake unconfigure
	$(MAKE) $(SILENT) -C $@/gmake configure 		\
		SETTINGS="METAGS_DIR=$(GMAKE_DIR)/$(THREAD)	\
		METAGS_LIB=$(GMAKE_METAGS_LIB) \
		METAGS_INC=$(GMAKE_METAGS_INC) \
		OBJDIR=$(GMAKE_OBJDIR) \
		CFLAGS=\"$(INCLUDES) -mmodel=large $(G_CFLAGS)\""
	$(MAKE) -C $@/gmake $(G_SUBMODS_GOALS)
	$(MAKE) -C $@/gmake unconfigure
endif
endif

###############################################
## Include auto-dependancies if not cleaning
## or building libs
## this works 'cos libs on cmdline doesn't
## get passed beyond thread
ifneq ($(MAKECMDGOALS),clean)
 ifneq ($(MAKECMDGOALS),libclean)
  ifneq ($(MAKECMDGOALS),libs)
   ifdef OBJECTS
    -include $(OBJECTS:%.o=%.d)
   endif
  endif
 endif
endif

###############################################
## FORCE rule: used as a pre-requisite to
##	force the action every time
##
FORCE: ;


###############################################
## additional make search paths
##
vpath %.a $(XLIBBUILDDIR)



###############################################
###############################################
##
## RULES:
##
###############################################
###############################################

###############################################
## .c -> .o rule - compile c file (and simultaneously create dependency file)
##
##
#$(XBUILDDIR)/%.o: $(SRCDIR)%.c
ifneq (,$(XGEN_TARGETDIR))
$(XBUILDDIR)/%.o: $(SRCDIR)%.c
else
$(XBUILDDIR)/%.o: %.c
endif
	set -e; rm -f $@; rm -f $(XBUILDDIR)/$(<F:%.c=%.d); \
	echo Compiling $(<F); \
	if [ $(MSDEV) ]; then \
		$(CC) -MMD -MM -nostdinc -MT $@ -E -w $(CFLAGS) $(GCCFLAGS) $(abspath $<) -c -o $(@:%.o=%.x); rm -f $(@:%.o=%.x); \
		if !([ -f $(XBUILDDIR)/$(<F:%.c=%.d) ]); then \
			sed 's,\($*\)\.o[ :]*,$(XBUILDDIR)/\1.o : ,g' < $(<F:%.c=%.d) > $(XBUILDDIR)/$(<F:%.c=%.d); \
			rm -f $(<F:%.c=%.d); \
		fi; \
		$(CL_EXE) $(CFLAGS) $(CLFLAGS) $< /Fo"$@" /Fd"$(XBUILDDIR)/vc70.pdb" /nologo /TC; \
		if [ $(THIS_LIB) ]; then \
			echo Adding $(@F) to $(notdir $(THIS_LIB)); \
			SAVEDIR=$(CURDIR); \
			cd $(XBUILDDIR); \
			if [ -e $(notdir $(THIS_LIB)) ]; then \
				$(LIB_EXE) /OUT:"$(THIS_LIB)" /NOLOGO $(notdir $(THIS_LIB)) $(<F:%.c=%.o); \
			else \
				$(LIB_EXE) /OUT:"$(THIS_LIB)" /NOLOGO $(<F:%.c=%.o); \
			fi; \
			cd $(SAVEDIR); \
		fi; \
	else \
		$(CC) -MMD -fno-strict-aliasing $(CFLAGS) $(abspath $<) -c -o $@; \
		if !([ -f $(XBUILDDIR)/$(<F:%.c=%.d) ]); then \
			sed 's,\($*\)\.o[ :]*,$(XBUILDDIR)/\1.o : ,g' < $(<F:%.c=%.d) > $(XBUILDDIR)/$(<F:%.c=%.d); \
			rm -f $(<F:%.c=%.d); \
		fi; \
		if [ $(THIS_LIB) ]; then \
			echo Adding $(@F) to $(notdir $(THIS_LIB)); \
			$(AR) r $(THIS_LIB) $@; \
			if [ $(GEN_COMBINED_LIBRARY) ]; then \
				echo Adding $(@F) to $(notdir $(GEN_COMBINED_LIBRARY)); \
				$(AR) r $(GEN_COMBINED_LIBRARY) $@; \
			fi; \
		fi; \
	fi; \
	if [ $(THIS_LIB) ]; then \
	    sed -e 's#$(<F:%.c=%.o)#$(subst .ax,.a,$(notdir $(THIS_LIB)))#' -e 's# \.\.# $(CURDIR)/\.\.#g' $(XBUILDDIR)/$(<F:%.c=%.d) > $(XBUILDDIR)/$(<F:%.c=%.ad);	\
	    echo -include $(XBUILDDIR)/$(<F:%.c=%.ad) >> '$(THIS_LIB)dd'; \
	fi;


###############################################
## .cpp -> .o rule - compile cpp file (and simultaneously create dependency file)
##
##
#$(XBUILDDIR)/%.o: $(SRCDIR)%.cpp
$(XBUILDDIR)/%.o: %.cpp
	set -e; rm -f $@; \
	echo Compiling $(<F); \
	if [ $(MSDEV) ]; then \
		$(CC) -MMD -MM -nostdinc++ -MT $@ -E -w $(CPPFLAGS) $(CFLAGS) $(GCCFLAGS) $(abspath $<) -c -o $(@:%.o=%.x); rm -f $(@:%.o=%.x); \
		if !([ -f $(XBUILDDIR)/$(<F:%.cpp=%.d) ]); then \
			sed 's,\($*\)\.o[ :]*,$(XBUILDDIR)/\1.o : ,g' < $(<F:%.cpp=%.d) > $(XBUILDDIR)/$(<F:%.cpp=%.d); \
			rm -f $(<F:%.c=%.d); \
		fi; \
		$(CL_EXE) $(CPPFLAGS) $(CFLAGS) $(CLFLAGS) $< /Fo"$@" /Fd"$(XBUILDDIR)/vc70.pdb" /nologo /TP; \
		if [ $(THIS_LIB) ]; then \
			echo Adding $(@F) to $(notdir $(THIS_LIB)); \
			SAVEDIR=$(CURDIR); \
			cd $(XBUILDDIR); \
			if [ -e $(notdir $(THIS_LIB)) ]; then \
				$(LIB_EXE) /OUT:"$(THIS_LIB)" /NOLOGO $(notdir $(THIS_LIB)) $(<F:%.cpp=%.o); \
			else \
				$(LIB_EXE) /OUT:"$(THIS_LIB)" /NOLOGO $(<F:%.cpp=%.o); \
			fi; \
			cd $(SAVEDIR); \
		fi; \
	else \
		$(CC) -MMD $(CPPFLAGS) $(CFLAGS) $(abspath $<) -c -o $@; \
		if !([ -f $(XBUILDDIR)/$(<F:%.cpp=%.d) ]); then \
			sed 's,\($*\)\.o[ :]*,$(XBUILDDIR)/\1.o : ,g' < $(<F:%.cpp=%.d) > $(XBUILDDIR)/$(<F:%.cpp=%.d); \
			rm -f $(<F:%.cpp=%.d); \
		fi; \
		if [ $(THIS_LIB) ]; then \
			echo Adding $(@F) to $(notdir $(THIS_LIB)); \
			$(AR) r $(THIS_LIB) $@; \
		fi; \
	fi; \
	if [ $(THIS_LIB) ]; then \
	    sed -e 's#$(<F:%.cpp=%.o)#$(subst .ax,.a,$(notdir $(THIS_LIB)))#' -e 's# \.\.# $(CURDIR)/\.\.#g' $(XBUILDDIR)/$(<F:%.cpp=%.d) > $(XBUILDDIR)/$(<F:%.cpp=%.ad);	\
	    echo -include $(XBUILDDIR)/$(<F:%.cpp=%.ad) >> '$(THIS_LIB)dd'; \
	fi;


###############################################
## .c.gz -> .c rule - uncompress a gzipped c file
##
##
$(SRCDIR)%.c: $(SRCDIR)%.c.gz
	echo Uncompressing $(<F)
	gzip -d -c $< > $@


###############################################
## .c.bz2 -> .c rule - uncompress a bzipped c file
##
##
$(SRCDIR)%.c: $(SRCDIR)%.c.bz2
	echo Uncompressing $(<F)
	bzip2 -d -c $< > $@


###############################################
## .o rule - compile s file
##
#$(XBUILDDIR)/%.o: $(SRCDIR)%.s
$(XBUILDDIR)/%.o: %.s
	set -e; rm -f $@; \
	echo Assembling $(<F); \
	$(AS) $(ASMINCLUDES) $(AFLAGS) $(INCLUDES) $(abspath $<) -o $@; \
	if [ $(THIS_LIB) ]; then \
	echo Adding $(@F) to $(notdir $(THIS_LIB)); \
	$(AR) r $(THIS_LIB) $@; fi


###############################################
## .o rule - generated c source
##
$(XBUILDDIR)/%.o: $(XBUILDDIR)/%.c
	set -e; rm -f $@; \
	echo Compiling $(<F); \
	$(CC) $(CFLAGS) $< -c -o $@
	if [ $(THIS_LIB) ]; then \
	echo Adding $(@F) to $(notdir $(THIS_LIB)); \
	$(AR) r $(THIS_LIB) $@; fi


###############################################
## .o rule - generated cpp source
##
$(XBUILDDIR)/%.o: $(XBUILDDIR)/%.cpp
	set -e; rm -f $@; \
	echo Compiling $(<F); \
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -c -o $@
	if [ $(THIS_LIB) ]; then \
	echo Adding $(@F) to $(notdir $(THIS_LIB)); \
	$(AR) r $(THIS_LIB) $@; fi


###############################################
## .a rule - build a lib from its components
##
##
$(XLIBBUILDDIR)%.a: $(G_SUBMODS) $(SUBLIBS) $(OBJECTS) $(GEN_OBJECTS)
	cp $(THIS_LIB) $@
	if [ ! $(MSDEV) ]; then \
		$(RANLIB) $@; \
	fi
	@echo .........$(@F): OK
	@echo ==================================

###############################################
## .lib rule - build a shared lib
##
##
$(BUILDDIR)%.lib: $(OBJECTS) $(GEN_OBJECTS) FORCE
	@echo ==================================
	@echo Linking $(@F)
	if [ -e temp/threads.lib.syms ]; then rm temp/threads.lib.syms; fi
	if [ -e ../thread0/temp/thread0.elf.syms ]; then cat ../thread0/temp/thread0.elf.syms >> temp/threads.lib.syms; fi
	if [ -e ../thread1/temp/thread1.elf.syms ]; then cat ../thread1/temp/thread1.elf.syms >> temp/threads.lib.syms; fi
	if [ -e ../thread2/temp/thread2.elf.syms ]; then cat ../thread2/temp/thread1.elf.syms >> temp/threads.lib.syms; fi
	if [ -e ../thread3/temp/thread3.elf.syms ]; then cat ../thread3/temp/thread1.elf.syms >> temp/threads.lib.syms; fi
	$(LD) $(LDFLAGS) -o $@ $(CRT1) $(OBJECTS) $(GEN_OBJECTS) $(CRT2)
	if [ -n "$(SHAREDLIBS_OUTDIR)" ]; then \
	if [ ! -d $(SHAREDLIBS_OUTDIR) ]; then mkdir -p $(SHAREDLIBS_OUTDIR); fi; \
	cp $@ $(SHAREDLIBS_OUTDIR); \
	fi
	@echo ==================================


# Convert LIBS to mixed format and filter to only include .a files
#ifneq (,$(ffindstring CYGWIN_NT,$(UNAME)))
#  LIBS_DOS=$(foreach xx, $(filter %.a,$(LIBS)), $(shell cygpath -u $(xx)))
#  MOD_LIBS_DOS=$(foreach xx, $(filter %.a,$(MOD_LIBS)), $(shell cygpath -u $(xx)))
#else
  LIBS_DOS := $(foreach xx, $(filter %.a,$(LIBS)), $(xx))
  MOD_LIBS_DOS := $(foreach xx, $(filter %.a,$(MOD_LIBS)), $(xx))
#endif

###############################################
## .exe rule
##
##$(warning OBJECTS**$(OBJECTS)**)
##$(warning GEN_OBJECTS**$(GEN_OBJECTS)**)
##$(warning LIBS_DOS**$(LIBS_DOS)**)
##$(warning APP_LIBS2**$(APP_LIBS2)**)
##$(warning APP_LIBS1**$(APP_LIBS1)**)
$(BUILDDIR)%.exe: $(OBJECTS) $(GEN_OBJECTS) $(LIBS_DOS) $(APP_LIBS2) $(APP_LIBS1)
	@echo ==================================
ifdef LINUX_BUILD
	echo Linking $(subst .exe,,$(@F))
	$(LD) $(LDFLAGS) -o $@ $(CRT1) $(APP_LIBS1) $(OBJECTS) $(GEN_OBJECTS) $(LIBS_DOS) $(APP_LIBS2) $(APP_LIBS_DOS) $(LIBS_DOS) $(CRT2) $(APP_LDFLAGS2)
	mv $@ $(subst .exe,,$@)
else
	echo Linking $(@F)
  ifdef MSDEV
	cygpath -w  $(APP_LIBS1) $(OBJECTS) $(GEN_OBJECTS) $(LIBS_DOS) $(APP_LIBS2) $(APP_LIBS_DOS) $(LIBS_DOS) > _files.txt
	$(LINK_EXE) $(LDFLAGS) /OUT:"$@" /PDB:"$(BUILDDIR)$(subst .exe,,$(@F)).pdb" $(CRT1) @_files.txt $(CRT2) $(APP_LDFLAGS2)
	rm _files.txt
  else
	$(LD) $(LDFLAGS) -o $@ $(CRT1) $(APP_LIBS1) $(OBJECTS) $(GEN_OBJECTS) $(LIBS_DOS) $(APP_LIBS2) $(APP_LIBS_DOS) $(LIBS_DOS) $(CRT2) $(APP_LDFLAGS2)
  endif
endif
	@echo ==================================

###############################################
## .dll rule
##
$(BUILDDIR)%.dll: $(OBJECTS) $(GEN_OBJECTS) $(LIBS_DOS) $(APP_LIBS2) $(APP_LIBS1)
	@echo ==================================
	echo Linking $(@F)
	if [ $(MSDEV) ]; then \
		cygpath -w  $(APP_LIBS1) $(OBJECTS) $(GEN_OBJECTS) $(LIBS_DOS) $(APP_LIBS2) $(APP_LIBS_DOS) $(LIBS_DOS) > _files.txt ; \
		echo using `which link`; \
		$(LINK_EXE) $(LDFLAGS) /OUT:"$@" /DEF:"$(DLLDEF)" /PDB:"$(BUILDDIR)$(subst .dll,,$(@F)).pdb" /IMPLIB:"$(BUILDDIR)$(subst .dll,,$(@F)).lib" $(CRT1) @_files.txt $(CRT2) $(APP_LDFLAGS2); \
		rm _files.txt ; \
	else \
		echo ERROR: dll target not supported; \
		exit -1 ; \
	fi
	@echo ==================================

###############################################
## .ko rule
##
$(BUILDDIR)$(KO_NAME).ko: $(KO_SRC)
	ln -sf $(MAK_DIR)/ko_make $(BUILDDIR)/Makefile;	\
	$(MAKE) -C $(BUILDDIR); \
	rm -f $(BUILDDIR)/Makefile;


###############################################
## .so rule
##
##$(warning OBJECTS**$(OBJECTS)**)
##$(warning GEN_OBJECTS**$(GEN_OBJECTS)**)
##$(warning LIBS_DOS**$(LIBS_DOS)**)
##$(warning APP_LIBS2**$(APP_LIBS2)**)
##$(warning APP_LIBS1**$(APP_LIBS1)**)
$(BUILDDIR)%.so: $(OBJECTS) $(GEN_OBJECTS) $(LIBS_DOS) $(APP_LIBS2) $(APP_LIBS1)
	@echo ==================================
	if ([ -e $@ ] && [ ! -w $@ ]); then \
		echo WARNING: Cannot link $(@F) as it is not writeable; \
	else \
		echo Linking $(@F); \
		$(LD) $(LDFLAGS) -shared -o $@ $(CRT1) $(APP_LIBS1) $(OBJECTS) $(GEN_OBJECTS) $(LIBS_DOS) $(APP_LIBS2) $(LIBS_DOS) $(CRT2) $(APP_LDFLAGS2);	\
	fi
	@echo ==================================

###############################################
## _mod.elf rule - Link using -Us and don't use CRT1 or crtn, output elf.
## This must be before the more generic .elf rule.
##
$(BUILDDIR)%_mod.elf: $(OBJECTS) $(MOD_LIBS_DOS)
	@echo "Linking $(@F) (for relocation/debug symbols)"
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) $(MOD_LIBS_DOS) $(CRT2_LIBS)
	$(LD) $(LDFLAGS) -r -o $@.rel $(OBJECTS) $(MOD_LIBS_DOS) $(CRT2_LIBS)


###############################################
## .elf rule
##
$(BUILDDIR)%.elf: $(OBJECTS) $(GEN_OBJECTS) $(LIBS_DOS) $(APP_LIBS2) $(APP_LIBS1)
	@echo ==================================
	if ([ -e $@ ] && [ ! -w $@ ]); then echo WARNING: Cannot link $(@F) as it is not writeable; fi
	echo Linking $(@F)
	$(LD) $(LDFLAGS) -o $@ $(CRT1) $(APP_LIBS1) $(OBJECTS) $(GEN_OBJECTS) $(LIBS_DOS) $(APP_LIBS2) $(LIBS_DOS) $(CRT2) $(APP_LDFLAGS2)
	if [ ! -d $(CPU_MTX) ]; then \
	    sed s/%1/$(@F)/ $(PROJ_MAK_DIR)/mtx.js >$(BUILDDIR)$(subst .elf,,$(@F)).js;	\
	else \
	    if [ ! -d $(CPU_META) ]; then \
	        sed s/%1/$(@F)/ $(PROJ_MAK_DIR)/meta.js >$(BUILDDIR)$(subst .elf,,$(@F)).js; \
	    else \
	        echo Unknown CPU when creating .elf file in rules.inc; \
	    fi \
	fi
	if [ $(POSTLOAD_SCRIPT) ]; then \
	    if [ -e $(PROJ_MAK_DIR)/$(POSTLOAD_SCRIPT) ]; then \
	    	echo "Using postload script '$(PROJ_MAK_DIR)/$(POSTLOAD_SCRIPT)'"; \
		    cat $(PROJ_MAK_DIR)/$(POSTLOAD_SCRIPT) >> $(BUILDDIR)$(subst .elf,,$(@F)).js; \
	    else \
	        echo "*** WARNING: Unable to locate specified postload script '$(PROJ_MAK_DIR)/$(POSTLOAD_SCRIPT)' ***"; \
	    fi \
	fi
	if [ $(PRELOAD_SCRIPT) ]; then \
	    if [ -e $(PROJ_MAK_DIR)/$(PRELOAD_SCRIPT) ]; then \
	    	echo "Using preload script '$(PROJ_MAK_DIR)/$(PRELOAD_SCRIPT)'"; \
		    cat $(PROJ_MAK_DIR)/$(PRELOAD_SCRIPT) >> $(BUILDDIR)$(subst .elf,,$(@F)).js; \
	    else \
	        echo "*** WARNING: Unable to locate specified preload script '$(PROJ_MAK_DIR)/$(PRELOAD_SCRIPT)' ***"; \
	    fi \
	fi	
	
	@echo ==================================


###############################################
## .sym.txt rule - Produce a text file to help with size optimisation.
##
$(BUILDDIR)%.sym.txt: $(BUILDDIR)%.elf
	@echo Generating symbol information from $(<F)
	nm -n -S $< | grep -v [$$] > $@


###############################################
## _mod.reloc rule - Generate data section relocation info.
##
##
$(BUILDDIR)%_mod.reloc: $(BUILDDIR)%_mod.elf
	@echo Creating $(@F)
	echo V3 > $@
	echo $(TEXT_SECTION_ORIGIN) | cut -dx -f2 >> $@
	echo $(DATA_SECTION_ORIGIN) | cut -dx -f2 >> $@
	readelf --relocs $<.rel | awk '/Relocation section/ { ok=0 } /[.]rela[.]data/ || /[.]rela[.]text/ { ok=1 } ok && /unrecognized: [012]/' | sed 's/\([0-9a-fA-F]*\).*unrecognized:\ \([012]\)\ *\([0-9a-fA-F]*\).*/\1 \2 \3/' >> $@


###############################################
## .reloc rule - Generate data section "relocation" info for static binary.
## This has no reloc table, but provides the addresses for text and data.
## The data section address is extracted from the elf file so that it does
## not need to be specified in the build.
##
$(BUILDDIR)%.reloc: $(BUILDDIR)%.elf
	@echo Creating $(@F)
	echo V1 > $@
	echo $(TEXT_SECTION_ORIGIN) | cut -dx -f2 >> $@
	readelf -S $< | grep [.]data | sed -r s/.\*PROGBITS[^0-9a-f]\*\([0-9a-f]\*\).\*/\\1/ >> $@


###############################################
## _mod.dnl module rule - Link using -Us and don't use CRT1 or crtn, output dnl.
## This must be before the more generic .dnl rule.
##
##
$(BUILDDIR)%_mod.dnl: $(OBJECTS) $(MOD_LIBS_DOS)
	@echo Linking $(@F)
	$(LD) $(LDFLAGS) --oformat metadnl -o $@ $(OBJECTS) $(MOD_LIBS_DOS) $(CRT2_LIBS)


###############################################
## .dnl rule - Link as for elf, but using -Us and output dnl.
##
##
$(BUILDDIR)%.dnl: $(OBJECTS) $(GEN_OBJECTS) $(LIBS_DOS) $(APP_LIBS2) $(APP_LIBS1)
	@echo Linking $(@F)
	$(LD) $(LDFLAGS) --oformat metadnl -o $@ $(CRT1) $(APP_LIBS1) $(OBJECTS) $(GEN_OBJECTS) $(LIBS_DOS) $(APP_LIBS2) $(LIBS_DOS) $(CRT2) $(APP_LDFLAGS2)


ifdef USE_BIN_C_BUILD_RULES

###############################################
## _bin.c module rule - Generate c source for relocatable module binary.
## This also generates a _bin.h file.
##
##
$(BUILDDIR)%_bin.c: $(BUILDDIR)%.reloc $(BUILDDIR)%.dnl
	@echo Creating relocatable $(@F)
	$(FIXDNL) $(BUILDDIR)$*.dnl
	if [ -z "$(DATA_SECTION_ORIGIN)" ]; then \
		$(DNL2C) -reloc -dlimit `nm $(BUILDDIR)$*.elf | grep '$$SECTIONBASE.$$stack' | cut -d' ' -f1` $(BUILDDIR)$* $(BINARY_NAME) $(patsubst %.c,%,$@); \
	else \
		$(DNL2C) -reloc $(BUILDDIR)$* $(BINARY_NAME) $(patsubst %.c,%,$@); \
	fi
	@echo ==================================


###############################################
## _bin.c rule - Generate c source for main (static) binary.
## This rule is selected instead of the previous _bin.c rule if there is no .reloc file.
## This also generates a _bin.h file.
##
##
$(BUILDDIR)%_bin.c: $(BUILDDIR)%.dnl
	@echo Creating static $(@F)
	$(FIXDNL) $(BUILDDIR)$*.dnl
	$(DNL2C) $(BUILDDIR)$* $(BINARY_NAME) $(patsubst %.c,%,$@)
	@echo ==================================

endif

###############################################
## Rules to rebuild libs
##

## if($(IMG_OS) == meos)
##    include meos rules
ifneq (,$(findstring meos,$(IMG_OS)))
 include $(MAK_DIR)/os_meos_rules.inc
endif

## ifdef MUST_DO_FIRST
##  libs depends on $(MUST_DO_FIRST)
ifdef MUST_DO_FIRST
  libs:: $(MUST_DO_FIRST)
endif


## if($(LIBS) != "")
##	libs depends on $(LIBS)
ifneq (,$(LIBS))
ifeq ($(GMAKEON), 1)
  libs:: gsubmods $(LIBS)
else
  libs:: $(LIBS)
endif
else
  libs::
endif


## if($(G_SUBMODS) != "")
##	gsubmods depends on $(G_SUBMODS)
gsubmods: $(G_SUBMODS)

# Mark libraries generated by gmake read only
gsubmods_mark_ro:
	@echo ===========================================
	@echo Mark libraries generated by gmake read only
	@echo ===========================================
	chmod a-w $(GMAKE_DIR)/$(THREAD)/lib/*.a

# Mark libraries generated by gmake writeable
gsubmods_mark_w:
	chmod u+w $(XBUILDDIR)/*.a


###############################################
###############################################
## Cleanup rules
##
.PHONY: clean_rpc_source
clean_rpc_source:
ifneq (,$(PROJ_DEFAULT_RPC_MODE))
	@echo Cleaning RPC headers
	rm -f -v `find $(IMG_WORKROOT)/rpc/$(PROJ_DEFAULT_RPC_MODE)/$(IMG_PROJ)/include \( -name '*_rpc.h' \)`
	@echo Done
	@echo Cleaning RPC sources
	rm -f -v `find $(IMG_WORKROOT) \( -name '*.c' -o -name '*.log' \) | egrep 'server/code|client/code'`
	@echo Done
endif

.PHONY: clean_combined
clean_combined:
	if [ $(GEN_COMBINED_LIBRARY) ]; then \
	  echo Deleting combined library $(GEN_COMBINED_LIBRARY); \
	  rm -fv $(GEN_COMBINED_LIBRARY); \
	fi

ifneq (,$(filter %.elf,$(XTARGETS)))
ifeq ($(GMAKEON), 1)
libclean:: $(LIBS) clean_rpc_source gsubclean clean_combined
else
libclean:: $(LIBS) clean_rpc_source clean_combined
endif
else
##libclean::
endif

ifneq (,$(filter %.exe,$(XTARGETS)))
ifeq ($(GMAKEON), 1)
libclean:: $(LIBS) clean_rpc_source gsubclean
else
libclean:: $(LIBS) clean_rpc_source
endif
else
##libclean::
endif

ifneq (,$(filter %.so,$(XTARGETS)))
ifeq ($(GMAKEON), 1)
libclean:: $(LIBS) clean_rpc_source gsubclean
else
libclean:: $(LIBS) clean_rpc_source
endif
else
##libclean::
endif

ifneq (,$(filter %.dll,$(XTARGETS)))
ifeq ($(GMAKEON), 1)
libclean:: $(LIBS) clean_rpc_source gsubclean
else
libclean:: $(LIBS) clean_rpc_source
endif
else
##libclean::
endif

libclean::
ifneq (,$(BUILDDIRABS))
	@echo rm -rf $(BUILDDIRABS)
	@rm -rf $(BUILDDIRABS)
else
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	echo "!!  ERROR1 IN BUILDDIRABS  !!"
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
endif

RMTARGETS = $(filter-out %.a,$(XTARGETS))
clean:
ifneq (,$(BUILDDIRABS))
	@echo  rm -rf $(BUILDDIRABS)
	@rm -rf $(BUILDDIRABS)
else
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	echo "!!  ERROR2 IN BUILDDIRABS  !!"
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
endif


## if($(G_SUBMODS) != "")
##	gsubmods depends on $(G_SUBMODS)
gsubclean: $(G_SUBMODS)
