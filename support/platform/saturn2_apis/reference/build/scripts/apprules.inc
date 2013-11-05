###############################################
###############################################
##
##	This file is included at the
##	end of an application makefile
##
###############################################
###############################################

ifndef START_TIME
  export START_TIME:="$(shell date +%s)"
endif

###############################################
## Create BUILDDIR
##
ifdef BUILDDIR
  ZXT := $(shell if [ ! -d $(BUILDDIR) ]; then mkdir -p $(BUILDDIR); fi; )
  ##$(warning **Creating: $(BUILDDIR)** )
endif

###############################################
## METAG_DEF defined for IMG_CPU mtx or meta
##
ifneq (,$(findstring meta,$(IMG_CPU)))
	export METAG_DEF = 1
endif

ifneq (,$(findstring mtx,$(IMG_CPU)))
	export METAG_DEF = 1
endif

###############################################
###############################################
###############################################
## Default rule
##	This rule is executed if no other
##	rule is specified.
##
##$(warning TARGETS**$(TARGETS)**)

default_rule: $(TARGETS)

###############################################
## FORCE rule: used as a pre-requisite to
##	force the action every time
##
FORCE: ;

###################################################
## DEBUG rule: tell everyone what we are building
##
DEBUGX:
	@echo
	@echo "============================================="
	@echo "============================================="
	@date
	@echo
	@echo "  CURDIR         = \"$(CURDIR)\" "
	@echo "  IMG_WORKROOT   = \"$(IMG_WORKROOT)\" "
	@echo "  IMG_CPU        = \"$(IMG_CPU)\" "
	@if [ $$IMG_CPU_VARIANT ] ; then \
	 echo "  IMG_CPU_VARIANT= \"$(IMG_CPU_VARIANT)\" "; fi
	@echo "  IMG_PROJ       = \"$(IMG_PROJ)\" "
	@echo "  IMG_HW         = \"$(IMG_HW)\" "
	@if [ $$CLK_FREQ ] ;then \
	 echo "  CLK_FREQ       = \"$(CLK_FREQ)\"	"; fi
	@if [ $$IMG_PLATFORM ] ;then \
	 echo "  IMG_PLATFORM   = \"$(IMG_PLATFORM)\"	"; fi
	@if [ $$IMG_META_FREQ ] ;then \
	 echo "  IMG_META_FREQ  = \"$(IMG_META_FREQ)\"	"; fi
	@if [ $$IMG_SYS_FREQ ] ;then \
	 echo "  IMG_SYS_FREQ   = \"$(IMG_SYS_FREQ)\"	"; fi
	@if [ $$IMG_DDR_FREQ ] ;then \
	 echo "  IMG_DDR_FREQ   = \"$(IMG_DDR_FREQ)\"	"; fi
	@if [ $$IMG_XTAL_FREQ ] ;then \
	 echo "  IMG_XTAL_FREQ  = \"$(IMG_XTAL_FREQ)\"	"; fi
	@if [ $$IMG_DDR ] ;then \
	 echo "  IMG_DDR        = \"$(IMG_DDR)\"	"; fi
	@if [ $$IMG_OS ] ;then \
	 echo "  IMG_OS         = \"$(IMG_OS)\" "; fi
	@if [ $$IMG_OS_VARIANT ] ;then \
	 echo "  IMG_OS_VARIANT = \"$(IMG_OS_VARIANT)\" "; fi
	@echo "  RELEASE        = \"$(RELEASE)\" "
	@if [ $$PROJ_MAK_DIR ] ;then \
	 echo "  PROJ_MAK_DIR   = \"$(PROJ_MAK_DIR)\"	"; fi
	@if [ $$METAG_DEF ] ;then \
	 echo "  COMPILER       = \"$(METAG_INST_ROOT)\" "; \
	 echo "  LINKER         = \""`$(LD) --version | sed 's/\r//'`"\" "; fi
	@if [ $$ARC ] ;then \
	 echo "  ARC            = \"$(ARC)\"	"; fi
	@if [ $$IMG_LINUX_KERNEL ] ;then \
	 echo "  IMG_LINUX_KERNEL = \"$(IMG_LINUX_KERNEL)\"	"; fi
	@echo "  CC             = \"$(CC)\" "
	@echo "============================================="
	@echo "============================================="

###############################################
## .exe rule
##

%.exe: DEBUGX FORCE
	@if ([ -d build ]); then \
	 echo;\
	 echo =============================================;\
	 echo Running make in directory \"build\" $*;\
	 echo =============================================;\
	 $(MAKE) $(SILENT) -C build $(MAKECMDGOALS) THREAD=$* TARGET_NAME=$* EXE_NAME=$*;\
	else \
	 echo;\
	 echo =============================================;\
	 echo Running make \"makefile_2\" $*;\
	 echo =============================================;\
	 $(MAKE) $(SILENT) -f makefile_2 $(MAKECMDGOALS) THREAD=$* TARGET_NAME=$* EXE_NAME=$*;\
	fi
	@echo "============================================="
	@echo $(START_TIME) `date +%s` | awk '{ printf "Elapsed time - %02d:%02d\n", ($$2 - $$1) / 60, ($$2 - $$1) % 60}'
	@echo "============================================="
	@echo " "


###############################################
## .dll rule
##

%.dll: DEBUGX FORCE
	@if ([ -d build ]); then \
	 echo;\
	 echo =============================================;\
	 echo Running make in directory \"build\" $*;\
	 echo =============================================;\
	 $(MAKE) $(SILENT) -C build $(MAKECMDGOALS) THREAD=$* TARGET_NAME=$* DLL_NAME=$*;\
	else \
	 echo;\
	 echo =============================================;\
	 echo Running make \"makefile_2\" $*;\
	 echo =============================================;\
	 $(MAKE) $(SILENT) -f makefile_2 $(MAKECMDGOALS) THREAD=$* TARGET_NAME=$* DLL_NAME=$*;\
	fi
	@echo "============================================="
	@echo $(START_TIME) `date +%s` | awk '{ printf "Elapsed time - %02d:%02d\n", ($$2 - $$1) / 60, ($$2 - $$1) % 60}'
	@echo "============================================="
	@echo " "

###############################################
## .lib rule
##

%.lib: DEBUGX FORCE
	@if ([ -d build ]); then \
	 echo;\
	 echo =============================================;\
	 echo Running make in directory \"build\" $*;\
	 echo =============================================;\
	 $(MAKE) $(SILENT) -C build $(MAKECMDGOALS) THREAD=$* TARGET_NAME=$* LIB_NAME=$*;\
	else \
	 echo;\
	 echo =============================================;\
	  echo Running make \"makefile_2\" $(MAKECMDGOALS) $*;\
	 echo =============================================;\
	 $(MAKE) $(SILENT) -f makefile_2 $(MAKECMDGOALS) THREAD=$* TARGET_NAME=$* LIB_NAME=$*;\
	fi
	@echo "============================================="
	@echo $(START_TIME) `date +%s` | awk '{ printf "Elapsed time - %02d:%02d\n", ($$2 - $$1) / 60, ($$2 - $$1) % 60}'
	@echo "============================================="
	@echo " "

	
###############################################
## .ko rule
##
%.ko: DEBUGX FORCE
	@if ([ -d build ]); then \
	 echo;\
	 echo =============================================;\
	 echo Running make in directory \"build\" $*;\
	 echo =============================================;\
	 $(MAKE) $(SILENT) -C build $(MAKECMDGOALS) THREAD=$* TARGET_NAME=$* KO_NAME=$*;\
	else \
	 echo;\
	 echo =============================================;\
	 echo Running make \"makefile_ko\" $*;\
	 echo =============================================;\
	 $(MAKE) $(SILENT) -f makefile_ko $(MAKECMDGOALS) THREAD=$* TARGET_NAME=$* KO_NAME=$*;\
	fi
	@echo "============================================="
	@echo $(START_TIME) `date +%s` | awk '{ printf "Elapsed time - %02d:%02d\n", ($$2 - $$1) / 60, ($$2 - $$1) % 60}'
	@echo "============================================="
	@echo " "

###############################################
## .so rule
##

%.so: DEBUGX FORCE
	@if ([ -d build ]); then \
	 echo;\
	 echo =============================================;\
	 echo Running make in directory \"build\" $*;\
	 echo =============================================;\
	 $(MAKE) $(SILENT) -C build $(MAKECMDGOALS) THREAD=$* TARGET_NAME=$* SO_NAME=$*;\
	else \
	 echo;\
	 echo =============================================;\
	 echo Running make \"makefile_2\" $*;\
	 echo =============================================;\
	 $(MAKE) $(SILENT) -f makefile_2 $(MAKECMDGOALS) THREAD=$* TARGET_NAME=$* SO_NAME=$*;\
	fi

###############################################
## .ldr rule & top_level .elf rule
##

%.elf: DEBUGX FORCE
	@echo
	@echo =============================================
	@echo Running make in directory \"$*\"
	@echo =============================================
	@if [ ! -e $*/makefile ] ;then \
	echo BUILD WARNING : Unable to find makefile in directory \'$(CURDIR)/$*\' \(the build system is attempting to invoke \'make\' here\); fi
	$(MAKE) $(SILENT) -C $* $(MAKECMDGOALS)
	@echo $(START_TIME) `date +%s` | awk '{ printf "Elapsed time - %02d:%02d\n", ($$2 - $$1) / 60, ($$2 - $$1) % 60}'
	@echo "============================================="
	@echo " "

########################################
## .img.tar -> .img rule - extract an img file
##
##
$(SRCDIR)%.img: $(SRCDIR)%.img.tar
	@echo Extracting img file from $(<F)
	tar -xf $< > $@


ifneq (,$(findstring CYGWIN_NT,$(shell uname)))
  IMG_DOS=$(foreach xx,$(IMG), $(shell cygpath -am $(xx)))
else
  IMG_DOS=$(foreach xx,$(IMG), $(xx))
endif
IMG_CMD = $(addprefix -T,$(IMG_DOS))
ifdef SEC_LDR
 ifneq (,$(findstring CYGWIN_NT,$(shell uname)))
  SEC_DOS=$(foreach xx,$(SEC_LDR), $(shell cygpath -am $(xx)))
 else
  SEC_DOS=$(foreach xx,$(SEC_LDR), $(xx))
 endif
 SEC_CMD = $(addprefix --loader,$(SEC_DOS))
endif
%.ldr: $(IMG) FORCE
	@echo Running LDLK....
	set -e ;export METAG_INST_ROOT=$(subst \,\\,$(METAG_INST_ROOT_LD))  ; \
	$(LDLK) --verbose -g $(IMG_CMD) $(SEC_CMD) $(APP_LDLKFLAGS) -o$(BUILDDIR)$@
	@echo
	@echo =============================================
	@echo Finished building \"$(BUILDDIR)$@\"
	@echo $(START_TIME) `date +%s` | awk '{ printf "Elapsed time - %02d:%02d\n", ($$2 - $$1) / 60, ($$2 - $$1) % 60}'
	@echo =============================================

%.img: FORCE
	@if [ ! -e $@ ] ; then 		\
		echo "**ERROR**  Can't find IMG file [ $@ ]";  fi


###############################################
## libs/clean rules
##

ELF_LIST = $(filter %.elf,$(TARGETS))
EXE_LIST = $(filter %.exe,$(TARGETS))
SO_LIST = $(filter %.so,$(TARGETS))
DLL_LIST = $(filter %.dll,$(TARGETS))
KO_LIST = $(filter %.ko,$(TARGETS))
LIB_LIST = $(filter %.lib,$(TARGETS))

libs libclean: $(ELF_LIST) $(EXE_LIST) $(DLL_LIST) $(KO_LIST) $(SO_LIST) $(LIB_LIST)

clean: $(ELF_LIST) $(EXE_LIST) $(DLL_LIST) $(KO_LIST) $(SO_LIST) $(LIB_LIST)

	@rm -rf $(BUILDDIR)


###############################################
## reginc rule - rebuild register includes
##
reginc:
	@if ([ -d $(IMG_WORKROOT)/register_includes/$(IMG_PROJ) ]); then \
		$(MAKE) $(SILENT) -C $(IMG_WORKROOT)/register_includes/$(IMG_PROJ); \
	elif ([ -d $(IMG_WORKROOT)/register_includes ]); then \
		$(MAKE) $(SILENT) -C $(IMG_WORKROOT)/register_includes; \
	else \
		echo ERROR: Cannot find reginc makefile; \
	fi

###############################################

###############################################
## cprunwithargs - copies runwithargs.js (CS6.x) or runwithargs.py (CS7+) from
## project make directory to executable target directory
###############################################
cprunwithargs:
	@if [ -e $(PROJ_MAK_DIR)/runwithargs.js ]; then \
	    cp -vf $(PROJ_MAK_DIR)/runwithargs.js $(RELEASE)/; \
	fi
	@if [ -e $(PROJ_MAK_DIR)/runwithargs.py ]; then \
		    cp -vf $(PROJ_MAK_DIR)/runwithargs.py $(RELEASE)/; \
	fi


###############################################
## cleanall rule - clean libs and app
##
cleanall:
	@export MAKECMDGOALS=$(subst cleanall,,$(MAKECMDGOALS))
	@$(MAKE) $(SILENT) libclean
	@$(MAKE) $(SILENT) clean
