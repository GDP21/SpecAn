# Tuner test application makefile
#	Copyright (C) Imagination Technologies Ltd.

UNIT_TYPE=APP
UNIT_NAME=test_app

include $(SMAKE_ROOT)/smakeinit.mk

C_SRC=test.c

GLOBAL_C_DEFINES+=GDMA_HW_IMPLEMENTATION=GDMA_SYSBUS
SATURN_PERIPHERALS_DIR=$(MOBILETV_ROOT)/support/platform/saturn_apis/user
GLOBAL_INCLUDES+=$(SATURN_PERIPHERALS_DIR)/include
GLOBAL_INCLUDES+=$(SATURN_PERIPHERALS_DIR)/include/meta
GLOBAL_FEATURES+=GIODRV_PERIPHERALS SCBM_CLOCK_XTAL1


MODS +=	\
	$(UCC_INST_ROOT)/src/uccRuntime \
	$(MOBILETV_ROOT)/support/common/tuners/Si2153

UNIT_LIB_PATHS+=$(SATURN_PERIPHERALS_DIR)/lib/$(notdir $(METAG_INST_ROOT))

# Import the test version of the drivers
UNIT_IMPORTS+=Saturn_test_mtp-0gcc


# Add Link origin to the Linker command line options
UNIT_LDFLAGS += -0x80900000

# Treat warnings as errors
#GLOBAL_OPTIONS+=ERROR_ON_WARNING

GLOBAL_OPERATING_SYSTEM=MEOS
# Set this to run Doxygen
export DOXYGEN=1

# Ensure that all builds have debug info in the test application.
UNIT_CFLAGS += -g
	
include $(SMAKE_ROOT)/smake.mk
