#
# Dummy tuner support
#

MODS+=$(MOBILETV_ROOT)/support/specAn/dummyTuner
GLOBAL_DEFINES+=DUMMY_TUNER

PLT_C_SRC+=tuner_dummy.c
PLT_TUNER_SRCDIRS=dummy
