UNIT_TYPE=APP
UNIT_NAME=HPCOMMSTESTMTX

SMAKE_ROOT=../../../../../../../ensigmaSDK/smake

#
# Use a default loader
# Need to set this before smakeinit.mk is included
#
GLOBAL_OPTIONS+=DEFAULT_LOADER

#
# Need to test global value variables
#
include $(SMAKE_ROOT)/smakeinit.mk

#
# mobileTV root (needed by some of the included fragments).
# This points to the mobileTV directory.
#
override MOBILETV_ROOT:=$(CURDIR)/../../../../../../..
export MOBILETV_ROOT

################################################################################

BOARDCFG=$(UNIT_DIR)/build/smake/comet

GLOBAL_OPERATING_SYSTEM=MEOS
GLOBAL_PROCESSOR=MTX
GLOBAL_UCC=310

GLOBAL_LDFLAGS += -0x80900000

COMET_PERIPHERALS_DIR=$(MOBILETV_ROOT)/support/platform/comet_apis/user

GLOBAL_INCLUDES+=$(COMET_PERIPHERALS_DIR)/include
GLOBAL_INCLUDES+=$(COMET_PERIPHERALS_DIR)/include/mtx
GLOBAL_INCLUDES+=$(MOBILETV_ROOT)/support/platform/comet_apis/reference/apis/comet/include

################################################################################

MODS+=$(UNIT_DIR)/../../../host_port/mtx \
      $(UNIT_DIR)/../../d2h_hostPort

################################################################################

# Include the test data helper source code
SRCDIRS=$(SRCDIR) $(SRCDIR1) $(UNIT_DIR)/../common/source/generic

C_SRC=HPCommsTestMtx.c \
      testData.c

include $(SMAKE_ROOT)/smake.mk
