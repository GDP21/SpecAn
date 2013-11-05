#
# Example SOC support
#

#############################################################################

ifeq ($(UNIT_TYPE),APP)

NEEDS_MEOS=1

GLOBAL_DEFINES+=CORE \
		        SOC_CONFIG

endif

#############################################################################

PLT_C_SRC+=soc_core.c
PLT_SOC_SRCDIRS=core
