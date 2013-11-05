UNIT_TYPE=APP
UNIT_NAME=HPCOMMSTESTMETA

SMAKE_ROOT=../../../../../../../ensigmaSDK/smake

#
# mobileTV root (needed by some of the included fragments).
# This points to the mobileTV directory.
#
override MOBILETV_ROOT:=$(CURDIR)/../../../../../../..
export MOBILETV_ROOT

################################################################################

GLOBAL_OPERATING_SYSTEM=MEOS
GLOBAL_PROCESSOR=META

COMET_PERIPHERALS_DIR=$(MOBILETV_ROOT)/support/platform/comet_apis/user

GLOBAL_INCLUDES+=$(COMET_PERIPHERALS_DIR)/include
GLOBAL_INCLUDES+=$(COMET_PERIPHERALS_DIR)/include/meta
GLOBAL_INCLUDES+=$(MOBILETV_ROOT)/support/platform/comet_apis/reference/apis/comet/include

################################################################################

MODS+=$(UNIT_DIR)/../../../host_port/meta \
      $(UNIT_DIR)/../../h2d_hostPort

################################################################################

# Include the test data helper source code
SRCDIRS=$(SRCDIR) $(SRCDIR1) $(UNIT_DIR)/../common/source/generic

C_SRC=HPCommsTestMeta.c \
      testData.c

include $(SMAKE_ROOT)/smake.mk