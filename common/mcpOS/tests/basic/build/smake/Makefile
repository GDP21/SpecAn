UNIT_TYPE=LOADER
UNIT_NAME=mcposBasic

SMAKE_ROOT=../../../../../../ensigmaSDK/smake
UCCP_ROOT=../../../../../uccPlatform
include $(UCCP_ROOT)/targetSetup.mk

APPS=$(UNIT_DIR)@MakefileMain

#
# For META, GRAM buffers are built into a separate ELF
#
ifeq ($(PROCESSOR),META)
APPS+=$(UNIT_DIR)@MakefileGRAM
IMGS+=mcposBasic.img
endif

include $(SMAKE_ROOT)/smake.mk
