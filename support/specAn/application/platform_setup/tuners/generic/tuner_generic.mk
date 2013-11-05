#
# Generic tuner support
#

MODS+=$(GENERIC_TUNER_ROOT)/tvtunerGeneric
GLOBAL_DEFINES+=GENERIC_TUNER

PLT_C_SRC+=tuner_generic.c
PLT_TUNER_SRCDIRS=generic
