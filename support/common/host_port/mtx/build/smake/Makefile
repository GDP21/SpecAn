UNIT_TYPE=MOD
UNIT_NAME=HPCLIENT

SRCDIR=$(UNIT_DIR)/source/generic

UNIT_HEADER=HPClient_api.h

UNIT_EXTRAHDRS=HPClient_drv.h

C_SRC=HPClient_api.c \
      HPClient_drv.c

UNIT_INCLUDES=$(UNIT_DIR)/../common/source/generic

include $(SMAKE_ROOT)/smake.mk

# Enable debug info for all builds. This allows the debugger to monitor variables.
UNIT_CFLAGS+= -g 
      
      
