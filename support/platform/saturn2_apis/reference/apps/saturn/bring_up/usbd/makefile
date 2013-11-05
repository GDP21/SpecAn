###############################################
###############################################
##
## MTX Firmware Makefile
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

export IMG_PROJ 	:= saturn
export IMG_OS 		:= meos_preinstalled

export RELEASE 		:= debug
export IMG_HW 		:= es1
export IMG_PLATFORM	:= hw

export GEN_MAPFILE 	:= 1

##export APP_CFLAGS = -Dxx

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
TARGETS = thread0.elf usbdtest.ldr

###############################################
##
##	IMG - .img file used by $(LDLK) to produce
##			an image.
##		- required if target contains a '.ldr'
##

IMG = $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_memory.img
include $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_setup_defaults.inc
IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_cache.img
IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_layout_$(RELEASE).img

###############################################
###############################################

###############################################
##
##	Include app rules
##
include $(MAK_DIR)/apprules.inc

