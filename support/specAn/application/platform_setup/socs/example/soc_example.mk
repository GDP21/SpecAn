#
# Example SOC support
#

#
# Uncomment out this line to define the UCCP here
#
#GLOBAL_UCCP=XYZ_C

#############################################################################

ifeq ($(UNIT_TYPE),LOADER)
IMGS_DIRL+=$(CODELINE_ROOT)/support/specAn/application/platform_setup/socs/example
IMGS+=soc_example.img
endif

#############################################################################

ifeq ($(UNIT_TYPE),APP)

NEEDS_MEOS=1

GLOBAL_DEFINES+=EXAMPLE \
		        SOC_CONFIG

endif

#############################################################################

PLT_C_SRC+=soc_example.c
PLT_SOC_SRCDIRS=example
