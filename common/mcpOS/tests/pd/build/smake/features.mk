ifneq ($(filter $(FEATURES), FAST_INTS),)
GLOBAL_CFLAGS+= -mintsys=mtp
GLOBAL_LDFLAGS+= -mintsys=mtp
GLOBAL_DEFINES+= __META_MTP_INTERRUPTS__
else
GLOBAL_CFLAGS+= -mintsys=atp
GLOBAL_LDFLAGS+= -mintsys=atp
GLOBAL_DEFINES+= __META_ATP_INTERRUPTS__
endif