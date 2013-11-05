#
# Example tuner support
#

MODS+=$(TUNERS_ROOT)/example_tuner
GLOBAL_DEFINES+=EXAMPLE_TUNER

PLT_C_SRC+=tuner_example.c
PLT_TUNER_SRCDIRS=example
