###############################################
###############################################
##
## UART Bootapp Makefile
##
###############################################
###############################################

###############################################
##
##	Build Options
##  =============
##
##	TARGET		- specify target
##				Targets currently supported:
##				CPU:	{meta|win32|mtx}
##				PROJ:	{msvdx}
##				HW:		{msvdx}
##
##	RELEASE		{debug|test|release}
##

export IMG_CPU 		:= mtp
export IMG_PROJ		:= saturn
export IMG_HW 		:= es1
export RELEASE 		:= release
export IMG_PLATFORM     := hw
export IMG_OS		:= meos_preinstalled

export GEN_MAPFILE := TRUE

## App needs to be in Minim - can only boot minim apps
export APP_LDFLAGS	= -mminim

###############################################
##
##	Convert IMG_WORKROOT to unix (if defined)
##	- otherwise make one up ;-)
##
ifdef IMG_WORKROOT
  export IMG_WORKROOT := $(shell cygpath -u $$IMG_WORKROOT)
else
  export IMG_WORKROOT := $(shell cd ../../../..; cygpath -au .)
endif


###############################################
##
##	Set location of standard make scripts
##
export MAK_DIR := $(IMG_WORKROOT)/build/scripts

export PROJ_MAK_DIR := $(IMG_WORKROOT)/build/$(IMG_PROJ)/meta


###############################################
##
##	Include generic makefile
##
include $(MAK_DIR)/make.inc


###############################################
##
##	TARGETS - list the final target(s) of this makefile.
##		Valid targets: ?.a ?.h ?.elf ?.ldr
##
##  The underlying THREAD system is used to generate the separate modules,
##  the .elf files are specified here as the targets to dupe the build system
##  into running the sub makefiles to produce their (real) targets.
##
##  We put reginc first here to force the register includes to be updated
##  before any dependency files are generated, as the dependency files
##  depend on the contents of the generated include files.
##
BUILDDIR := $(RELEASE)/
TARGETS = 	thread0.elf \
		saturn_bootapp \

###############################################
##
##	IMG - .img file used by $(LDLK) to produce
##			an image.
##		- required if target contains a '.ldr'
##

ifeq ($(IMG_PLATFORM), fpga)
IMG = $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_memory_boot.img
IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_cache.img
IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_setup_fpga_boot.img
IMG += $(PROJ_MAK_DIR)/boot_app_gram.img
else
IMG = $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_memory_boot.img
IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_cache.img
IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_setup_400mhz_meta__200mhz_sys__400mhz_ddr__48mhz_xtal__64mb_ddr2_boot.img
IMG += $(PROJ_MAK_DIR)/boot_app_ddr.img
endif

## Prepend -T's to each img file
IMG_CMD = $(addprefix -T, $(IMG))


###############################################
###############################################

###############################################
##
##	Include app rules
##
include $(MAK_DIR)/apprules.inc

ifdef PAGE_SIZE 
  ifdef VPAGE_SIZE
	PAGEOPTS = --pagesize $(PAGE_SIZE) --vpagesize $(VPAGE_SIZE)
	PAGESTRING = _$(PAGE_SIZE)_$(VPAGE_SIZE)
  else
	PAGEOPTS = --pagesize $(PAGE_SIZE)
	PAGESTRING = _$(PAGE_SIZE)
  endif
else
  ifdef VPAGE_SIZE
	PAGEOPTS = --vpagesize $(VPAGE_SIZE)
	PAGESTRING = _$(VPAGE_SIZE)
  endif
endif

PAGESTRING_STRIPPED = $(strip $(PAGESTRING))
SECLOADBIN = $(PROJ_MAK_DIR)/../../../bootrom/saturn/boot/secload/gmake/secload.bin
SECLOADBINSL = $(PROJ_MAK_DIR)/../../../bootrom/saturn/boot/secload/gmake/secloadsl.bin
LDLK_OPTS = -g --fixed-loader=0xB71BB000 -mminim --minim-stride=0

saturn_bootapp:
	@echo Custom build step; \
	if [ -e $(SECLOADBIN) ]; then \
		$(METAG_INST_ROOT)/metag-local/bin/ldlk $(LDLK_OPTS) --loader$(SECLOADBIN) -Map$(RELEASE)/uart_bootapp$(PAGESTRING_STRIPPED).map $(IMG_CMD) -o$(RELEASE)/uart_bootapp$(PAGESTRING_STRIPPED).ldr --verbose $(PAGEOPTS); \
	else \
		echo "ERROR: Cannot find secondary loader binary"; \
	fi; \
	if [ -e $(SECLOADBINSL) ]; then \
		$(METAG_INST_ROOT)/metag-local/bin/ldlk $(LDLK_OPTS) --loader$(SECLOADBINSL) -Map$(RELEASE)/uart_bootapp_sl$(PAGESTRING_STRIPPED).map $(IMG_CMD) -o$(RELEASE)/uart_bootapp_sl$(PAGESTRING_STRIPPED).ldr --verbose $(PAGEOPTS); \
	else \
		echo "ERROR: Cannot fid slave secondary loader binary"; \
	fi

flash_images:
	@echo Building nvram sim images; \
	if [ -e ./bin2spimsim.exe ]; then \
		if [ -e release/uart_bootapp.ldr ]; then \
			./bin2spimsim.exe release/uart_bootapp.ldr > flashImageNumonyx.txt; \
		fi; \
		if [ -e release/uart_bootapp_264.ldr ]; then \
			./bin2spimsim.exe release/uart_bootapp_264.ldr > flashImageAtmel264.txt; \
		fi; \
		if [ -e release/uart_bootapp_528.ldr ]; then \
			./bin2spimsim.exe release/uart_bootapp_528.ldr > flashImageAtmel528.txt; \
		fi; \
	else \
		echo "ERROR: Cannot find bin2spimsim.exe tool in current directory"; \
	fi;
