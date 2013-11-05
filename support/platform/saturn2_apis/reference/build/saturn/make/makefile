###############################################
###############################################
##
## General Saturn library build makefile
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
##				CPU:	{meta}
##				PROJ:	{saturn}
##				HW:	{es1}
##				PLATFORM: {fpga|emu|hw}
##
##	RELEASE		{debug|test|release}
##

## Settings unlikely to change throughout project lifetime
export IMG_CPU		:= mtp
export IMG_PROJ		:= saturn
export IMG_OS		:= meos_preinstalled

## Settings likely to change
export RELEASE		:= debug
export IMG_HW		:= es1
export IMG_PLATFORM	:= hw
export THREAD_ID 	:= 0

export GEN_MAPFILE	:= 1

###############################################
##
##	Check IMG_WORKROOT is set
##
ifndef IMG_WORKROOT
  $(error *** IMG_WORKROOT not set ***)
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

TARGETS = thread$(THREAD_ID).elf

IMG  = $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_memory.img
ifneq ($(IMG_HW), saturn_boot)
include $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_setup_defaults.inc
endif
IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_cache.img
IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_map.img
IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_layout_$(RELEASE).img

###############################################
##
##	Include app rules
##
include $(MAK_DIR)/apprules.inc



