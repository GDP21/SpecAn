UNIT_TYPE=MOD
UNIT_NAME=D2H_DASH

SRCDIR=$(UNIT_DIR)/code

UNIT_HEADER=d2h_api.h

UNIT_EXTRAHDRS=img_common.h \
               img_defs.h \
	       img_types.h \
	       dacomms_api.h

C_SRC=d2h_api.c

UNIT_INCLUDES=$(UNIT_DIR)/../$(TARGET)_include \
	      $(IMG_WORKROOT)/include \
              $(IMG_WORKROOT)/include/mtx \
	      $(METAG_INST_ROOT)/mtxg-local/include/meos/ \
	      $(UNIT_DIR)/../../../include

include $(SMAKE_ROOT)/smake.mk

# Enable debug info for all builds. This allows the debugger to monitor variables.
UNIT_CFLAGS+= -g 
      
      
