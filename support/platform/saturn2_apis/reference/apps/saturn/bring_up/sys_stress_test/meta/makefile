###############################################
###############################################
##
##  Sys Stress Test Makefile (META)
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
##				
##
##	RELEASE		{debug|test|release}
##

export IMG_CPU 		:= meta
export IMG_PROJ 	:= saturn
export RELEASE 		:= debug
export IMG_OS 		:= none
export IMG_HW 		:= es1
export IMG_PLATFORM	:= hw

export PROC_ID 	:= processor2

##export GEN_MAPFILE := 1
export APP_LDFLAGS := -Map=../$(RELEASE)/thread0.elf.map:9 -MM

##export COUNTER_API_B_RPC_MODE := ipc_tbi

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


###############################################
##
##	Include generic makefile
##
include $(MAK_DIR)/make.inc

###############################################
##
##	Include Stress Test versions of IPC setup
##

##export APP_CFLAGS := -I$(IMG_WORKROOT)/apps/comet/bring-up/sys_stress_test/ipc_support

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
#TARGETS = thread0.elf threadg.elf threadl.elf sys_stress_test.ldr
TARGETS = thread0.elf threadg.elf sys_stress_test.ldr

###############################################
##
##	IMG - .img file used by $(LDLK) to produce
##			an image.
##		- required if target contains a '.ldr'
##

ifeq ($(IMG_CPU), meta)

## DDR VERSION
IMG = $(IMG_WORKROOT)/build/$(IMG_PROJ)/meta/$(IMG_HW)/$(IMG_PLATFORM)/perip_memory.img
IMG += stress_test_perip_setup.img
include $(IMG_WORKROOT)/build/$(IMG_PROJ)/meta/$(IMG_HW)/$(IMG_PLATFORM)/perip_setup_defaults.inc
IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_cache.img
IMG += stress_test_perip_layout_$(RELEASE).img

## GRAM VERSION
#IMG = $(IMG_WORKROOT)/build/$(IMG_PROJ)/meta/$(IMG_HW)/$(IMG_PLATFORM)/perip_memory_sram_only.img
#IMG += stress_test_perip_setup_gram_only.img
#include $(IMG_WORKROOT)/build/$(IMG_PROJ)/meta/$(IMG_HW)/$(IMG_PLATFORM)/perip_setup_defaults_gram_only.inc
#IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_cache.img
#IMG += stress_test_perip_layout_gram_only_$(RELEASE).img

else
$(info *** This Test must be built for META ***)
endif



###############################################
###############################################

###############################################
##
##	Include app rules
##
include $(MAK_DIR)/apprules.inc

