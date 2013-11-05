UNIT_TYPE=MOD
UNIT_NAME=maths_funcs

include $(SMAKE_ROOT)/smakeinit.mk

UNIT_EXTRAHDRS=
UNIT_HEADER=maths_funcs.h

ifeq ($(PROCESSOR),MTX)
	UNIT_DEFINES+=MTX
endif

C_SRC= \
		add32.c \
		core.c \
		div64.c \
		divq16.c \
		log10.c \
		log2.c \
		restrict_range.c \
		round_scale.c \
		round_shift.c
		
UNIT_OPTIONS+=ERROR_ON_WARNING

include $(SMAKE_ROOT)/smake.mk

