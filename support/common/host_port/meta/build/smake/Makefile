UNIT_TYPE=MOD
UNIT_NAME=HPSERVER

SRCDIR=$(UNIT_DIR)/source/generic

UNIT_HEADER=HPServer_api.h

UNIT_EXTRAHDRS=HPServer_drv.h

C_SRC=HPServer_api.c \
      HPServer_drv.c

UNIT_INCLUDES=$(UNIT_DIR)/../common/source/generic

include $(SMAKE_ROOT)/smake.mk

# Enable debug info for all builds. This allows the debugger to monitor variables.
UNIT_CFLAGS+= -g 
      
      
