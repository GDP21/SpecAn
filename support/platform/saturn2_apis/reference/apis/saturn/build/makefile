
include $(MAK_DIR)/make.inc

BUILDDIR=../lib/$(IMG_HW)/$(THREAD)/$(RELEASE)/
SUBPATH=../libraries/

ifneq ($(IMG_HW), saturn_boot)
  SUBLIBS = sys_config	\
			sys_util	\
			block_desc	\
			nvram_api	\
			gpio
else
  SUBLIBS = sys_util	\
			block_desc
endif

## Check if DACOMMS should be built
ifdef DACOMMS_VARIANT
  SUBLIBS+=$(SUBPATH)/dacomms/$(DACOMMS_VARIANT)
endif

TARGETS = libSaturnAPI_$(THREAD).a

include $(MAK_DIR)/rules.inc
