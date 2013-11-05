# Common AGC test application makefile
#	Copyright (C) Imagination Technologies Ltd.

UNIT_TYPE=APP
UNIT_NAME=agc_test_app

include $(SMAKE_ROOT)/smakeinit.mk

C_SRC=agc_test.c

GLOBAL_FEATURES+=MSI_SPIM MSI_001

# Doxygen will only be created if the code actually builds...
# The following defines are only to make it build and don't affect the APIs
GLOBAL_DEFINES+=PLAYOUT TDMB NEW_UCC_DRIVER

# treat input as complex samples.
UNIT_DEFINES+=COMPLEX_INPUT

MODS +=	\
	$(MOBILETV_ROOT)/common/uccToolkit/uccRuntime \
	$(MOBILETV_ROOT)/common/maths_funcs \
	$(MOBILETV_ROOT)/common/agc

# Add Link origin to the Linker command line options
UNIT_LDFLAGS += -Wl,-0x80900000

# Treat warnings as errors
GLOBAL_OPTIONS+=ERROR_ON_WARNING

# Set this to run Doxygen
export DOXYGEN=1

# Ensure that all builds have debug info in the test application.
UNIT_CFLAGS += -g
	
include $(SMAKE_ROOT)/smake.mk
