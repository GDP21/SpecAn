# Tuner test application makefile
#	Copyright (C) Imagination Technologies Ltd.

UNIT_TYPE=APP
UNIT_NAME=test_app

include $(SMAKE_ROOT)/smakeinit.mk

C_SRC=test.c

GLOBAL_C_DEFINES+=GDMA_HW_IMPLEMENTATION=GDMA_SYSBUS
COMET_PERIPHERALS_DIR=$(MOBILETV_ROOT)/support/platform/comet_apis/user
GLOBAL_INCLUDES+=$(COMET_PERIPHERALS_DIR)/include
GLOBAL_INCLUDES+=$(COMET_PERIPHERALS_DIR)/include/mtx
GLOBAL_FEATURES+=GIODRV_PERIPHERALS SCBM_CLOCK_XTAL1

MODS +=	\
	$(MOBILETV_ROOT)/common/uccDriver_UCCP$(UCC)/mod-uccDriver \
	$(MOBILETV_ROOT)/common/tuner_api \
	$(MOBILETV_ROOT)/support/common/tuners/max3543

# Add Link origin to the Linker command line options
UNIT_LDFLAGS += -0x80900000

# Treat warnings as errors
GLOBAL_OPTIONS+=ERROR_ON_WARNING

# Set this to run Doxygen
export DOXYGEN=1

# Ensure that all builds have debug info in the test application.
UNIT_CFLAGS += -g
	
include $(SMAKE_ROOT)/smake.mk
