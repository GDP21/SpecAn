UNIT_TYPE=MOD
UNIT_NAME=fk160x_tuner

C_SRC=fk160x_tuner.c fk160x_scbm$(PORT_DRIVER_SUFFIX).c

SRCDIR1=$(UNIT_DIR)/source/generic/tmbslTDA18273

C_SRC+=tmbslTDA18273.c \
		tmbslTDA18273_Instance.c \
		tmbslTDA18273_RegDef.c
		
# to match the documentation the TDA18273 driver requires the following define
UNIT_DEFINES+=_TVFE_SW_ARCH_V4		

#GLOBAL_OPTIONS+=ERROR_ON_WARNING

include $(SMAKE_ROOT)/smake.mk
