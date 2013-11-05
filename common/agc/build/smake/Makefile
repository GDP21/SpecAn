UNIT_TYPE=MOD
UNIT_NAME=common_agc

include $(SMAKE_ROOT)/smakeinit.mk

UNIT_EXTRAHDRS=
UNIT_HEADER=common_agc.h

ifeq ($(PROCESSOR),MTX)
	UNIT_DEFINES+=MTX
endif

C_SRC= \
		calc_dcoffset.c \
		calc_gain.c \
		calc_iqoffset.c \
		calc_loops.c \
		common_agc.c
        
UNIT_OPTIONS+=ERROR_ON_WARNING

include $(SMAKE_ROOT)/smake.mk
