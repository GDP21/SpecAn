UNIT_TYPE=MOD
UNIT_NAME=plt_setup

include $(SMAKE_ROOT)/smakeinit.mk

SRCDIRS=$(UNIT_DIR)/source/generic \
		$(UNIT_DIR)/scp

ifdef PLT_TUNER_SRCDIRS
SRCDIRS+=$(addprefix $(UNIT_DIR)/tuners/,$(PLT_TUNER_SRCDIRS))
endif
ifdef PLT_SOC_SRCDIRS
SRCDIRS+=$(addprefix $(UNIT_DIR)/socs/,$(PLT_SOC_SRCDIRS))
endif
ifdef PLT_TARGET_SRCDIRS
SRCDIRS+=$(addprefix $(UNIT_DIR)/targets/,$(PLT_TARGET_SRCDIRS))
endif

#$(warning *** SRCDIRS $(SRCDIRS))

C_SRC=plt_setup.c

ifdef PLT_C_SRC
C_SRC+=$(PLT_C_SRC)
endif

ifdef IF
C_SRC+=scp_$(IF).c
endif

ifeq ($(PROCESSOR),MTX)
UNIT_DEFINES+=MTX
endif

include $(SMAKE_ROOT)/smake.mk
