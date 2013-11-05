UNIT_TYPE=PKG
UNIT_NAME=SPEC_AN

PKG_ROOT=$(UNIT_DIR)/../../..

PKG_FILTER_INCLUDES=__NONE__

PKG_COMPS=support/specAn/loaders/demoApp

ifeq ($(PKG_CONFIGURATION), internal)
$(warning internal build)
# For the internal package we build main app as well as host port app;
# easier for testing.  Note that "internal" is a special package name, smake
# overrides its default of building without source.
PKG_COMPS+=support/specAn/loaders/demoApp

# Assume TARGET=SATURN2_BUB
export BUILD_FOR_BOOT_LOADER=1
PKG_FOLDERS+=support/platform/saturn2_apis/user/lib/$(notdir $(METAG_INST_ROOT))
PKG_FILES+=support/specAn/loaders/demoApp/source/meta/SpecAn_330_3.ldscript
endif

SMAKE_ROOT=$(UCC_INST_ROOT)/ensigmaSDK/smake
include $(SMAKE_ROOT)/smake.mk
