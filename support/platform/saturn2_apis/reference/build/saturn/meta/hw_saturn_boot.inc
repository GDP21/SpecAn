###############################################
###############################################
##
##	SATURN_BOOT HW Target
##
##

CFLAGS += -mdtors-section=.bootdtors
CFLAGS += -mctors-section=.bootctors
CFLAGS += -mrodata-section=.bootrodata
CFLAGS += -mtext-section=.boottext
CFLAGS += -mdata-section=.bootdata

CFLAGS += -DBOOT_CODE
CFLAGS += -DUSBD_BOOT_CODE
CFLAGS += -DUSBD_NO_CBMAN_SUPPORT
export NO_DFU_SUPPORT=true

## Should be able to take the following two lines out if building with 2.4.0b2 toolkit
CFLAGS += -mminim
LDFLAGS += -mminim
