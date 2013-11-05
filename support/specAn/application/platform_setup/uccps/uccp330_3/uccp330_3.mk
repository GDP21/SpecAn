#
# UCCP330_3 support
#

#############################################################################

UCCP_ABS_ROOT=$(UCCLIB_ROOT)/uccPlatform
include $(UCCP_ABS_ROOT)/uccpSetup.mk

#############################################################################

ifeq ($(UNIT_TYPE),APP)
GLOBAL_DEFINES+=PLT_SUPPORTS_MEOS POST_SHANNON_DFE \
    CONSTELLATION_AMP_PRE_SHANNON=false
endif

#############################################################################

PLT_UCCP_SRCDIRS=uccp$(UCCP)

#############################################################################
