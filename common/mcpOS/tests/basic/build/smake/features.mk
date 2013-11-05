ifneq ($(filter $(FEATURES), FAST_INTS),)
#
# This test doesn't use MeOS so fast interrupts aren't supported.
# However, the toolkit distribution build scripts mechanically try to build this
# both with and without the FAST_INTS feature.
#
# As a workaround, we just select normal interrupt support, even when fast
# interrupts are requested.
#
$(info Ignoring FAST_INTS feature selection for mcpOS basic test)
#GLOBAL_CFLAGS+= -mintsys=mtp
#GLOBAL_LDFLAGS+= -mintsys=mtp
#GLOBAL_DEFINES+= __META_MTP_INTERRUPTS__
GLOBAL_CFLAGS+= -mintsys=atp
GLOBAL_LDFLAGS+= -mintsys=atp
GLOBAL_DEFINES+= __META_ATP_INTERRUPTS__
else
GLOBAL_CFLAGS+= -mintsys=atp
GLOBAL_LDFLAGS+= -mintsys=atp
GLOBAL_DEFINES+= __META_ATP_INTERRUPTS__
endif