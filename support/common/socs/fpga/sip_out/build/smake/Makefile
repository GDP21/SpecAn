UNIT_TYPE=MOD
UNIT_NAME=sip_out

include $(SMAKE_ROOT)/smakeinit.mk

UNIT_EXTRAHDRS=
UNIT_HEADER=sip_out.h

ifeq ($(PROCESSOR),MTX)
	UNIT_DEFINES+=MTX
endif

C_SRC= sip_out.c # etc...
        
UNIT_OPTIONS+=ERROR_ON_WARNING

include $(SMAKE_ROOT)/smake.mk
