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
GLOBAL_FEATURES+=MXL5007T
#GLOBAL_FEATURES+=MXL201RF

MODS +=	\
	$(MOBILETV_ROOT)/common/uccToolkit/uccRuntime \
	$(MOBILETV_ROOT)/support/common/tuners/mxl_dtv

UNIT_LIB_PATHS+=$(COMET_PERIPHERALS_DIR)/lib/$(notdir $(METAG_INST_ROOT))

# Import the release version of the drivers
ifeq ($(PROCESSOR),META)
UNIT_IMPORTS+=Comet_metagcc
endif

ifeq ($(PROCESSOR),MTX)
# We will run on the UCC1 within Comet, hence mtx1.
UNIT_IMPORTS+=Comet_test_mtx1gcc
endif
# Add Link origin to the Linker command line options
UNIT_LDFLAGS += -0x80900000

# Treat warnings as errors
GLOBAL_OPTIONS+=ERROR_ON_WARNING

GLOBAL_OPERATING_SYSTEM=MEOS
# Set this to run Doxygen
export DOXYGEN=1

# Ensure that all builds have debug info in the test application.
UNIT_CFLAGS += -g
	
include $(SMAKE_ROOT)/smake.mk
