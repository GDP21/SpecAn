UNIT_TYPE=MOD
UNIT_NAME=D2H_HOSTPORT

SRCDIR=$(UNIT_DIR)/source/generic

UNIT_HEADER=d2h_api.h

UNIT_EXTRAHDRS=../../../common/source/generic/dacomms_api.h

C_SRC=d2h_api.c

UNIT_INCLUDES=$(UNIT_DIR)/../../host_port/mtx/source/generic

include $(SMAKE_ROOT)/smake.mk

# Enable debug info for all builds. This allows the debugger to monitor variables.
UNIT_CFLAGS+= -g 
      
      
