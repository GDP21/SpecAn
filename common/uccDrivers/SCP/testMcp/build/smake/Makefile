UNIT_TYPE=LOADER
UNIT_NAME=scpDriverTest

SMAKE_ROOT=../../../../../../ensigmaSDK/smake

# Use the UCC lib in the tools we are using.
UCCLIB_ROOT=$(UCC_INST_ROOT)/src
export UCCLIB_ROOT

UCCP_ROOT= $(UCCLIB_ROOT)/uccPlatform
UCCRT_ROOT=$(UCCLIB_ROOT)/uccRuntime
include $(UCCP_ROOT)/targetSetup.mk
GLOBAL_INCLUDES+=$(UCCRT_ROOT)/source/generic
GLOBAL_INCLUDES+=$(UCCP_ROOT)/source/generic

APPS=$(UNIT_DIR)@MakefileMain

#
# For META, GRAM buffers are built into a separate ELF
#
ifeq ($(PROCESSOR),META)
APPS+=$(UNIT_DIR)@MakefileGram
IMGS+=scpDriverTest.img
endif

include $(SMAKE_ROOT)/smake.mk
