UNIT_TYPE=APP
UNIT_NAME=demod_swapper

SMAKE_ROOT=../../../../../../ensigmaSDK/smake

#
# mobileTV root (needed by some of the included fragments).
# This points to the mobileTV directory.
#
override MOBILETV_ROOT:=$(CURDIR)/../../../../../..
export MOBILETV_ROOT

################################################################################

GLOBAL_UCC=UCC310
GLOBAL_TARGET=comet
GLOBAL_OPERATING_SYSTEM=MEOS
GLOBAL_PROCESSOR=META
GLOBAL_PROCESSOR_VERSION = 2.1                     #smake uses a default othewise and link flags below (esp -mmetac=2.1) appear ignored by smake and error results
                                                   #METAC 2 1  is also defined at top of memory.img

COMET_PERIPHERALS_DIR=$(MOBILETV_ROOT)/support/platform/comet_apis/user

GLOBAL_INCLUDES+=$(COMET_PERIPHERALS_DIR)/include
GLOBAL_INCLUDES+=$(COMET_PERIPHERALS_DIR)/include/meta
GLOBAL_INCLUDES+=$(MOBILETV_ROOT)/support/platform/comet_apis/reference/apis/comet/include

################################################################################

SRCDIR=$(UNIT_DIR)/thread0/code

C_SRC=demod_swapper.c

include $(SMAKE_ROOT)/smake.mk