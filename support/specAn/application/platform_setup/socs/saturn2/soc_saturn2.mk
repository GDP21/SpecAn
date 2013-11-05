#
# Saturn 2 SOC support
#

GLOBAL_UCCP=330_3

################################################################################

ifeq ($(UNIT_TYPE),APP)

#$(warning soc_saturn2)

UCCP_PERIPHERALS_DIR=$(CODELINE_ROOT)/support/platform/saturn2_apis/user
GLOBAL_INCLUDES+=$(UCCP_PERIPHERALS_DIR)/include
GLOBAL_INCLUDES+=$(UCCP_PERIPHERALS_DIR)/include/meta

# The end of the METAG_INST_ROOT is the dir for the tools version, use notdir to extract the last part
UNIT_LIB_PATHS+=$(UCCP_PERIPHERALS_DIR)/lib/$(notdir $(METAG_INST_ROOT))

UNIT_IMPORTS+=Saturn_test_mtp-0gcc

NEEDS_MEOS=1

GLOBAL_DEFINES+=SATURN2 \
		        SOC_CONFIG

GLOBAL_FEATURES+=GIODRV_PERIPHERALS SYS_CLOCK_200MHZ

endif

################################################################################

PLT_C_SRC+=soc_saturn2.c
PLT_SOC_SRCDIRS=saturn2
