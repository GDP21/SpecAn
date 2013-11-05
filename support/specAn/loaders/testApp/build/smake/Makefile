UNIT_TYPE=LOADER
UNIT_NAME=SPECAN

################################################################################################
##
## PATH CONFIGURATION SECTION
##
## The following two definitions locate
## 1) The UCC library source modules, (ucc runtime, platform control, common drivers etc)
## 2) The ensigma SDK
##
## These default to the versions in the current UCC toolkit - i.e. derived from UCC_INST_ROOT.
## Either or both of them may be overridden on the command line in order to build with non-standard
## versions of the support software
##
##
export UCCLIB_ROOT=$(UCC_INST_ROOT)/src
export ENSIGMA_SDK_ROOT=$(UCC_INST_ROOT)/ensigmaSDK
##
## Below here are necessary derived variables which are not intended to be overridden directly
##
UCCP_ROOT=$(UCCLIB_ROOT)/uccPlatform
SMAKE_ROOT=$(ENSIGMA_SDK_ROOT)/smake
##
##
##  END OF PATH CONFIGURATION SECTION
##
###################################################################################################

include $(SMAKE_ROOT)/smakeinit.mk

# Set up variables defining module paths
ifdef P4
export CODELINE_ROOT:=$(shell cd ../../../../..; pwd)
include $(CODELINE_ROOT)/support/application/common/build/smake/paths.mk
else
export CODELINE_ROOT:=$(shell cd ../../../../../..; pwd)
include $(CODELINE_ROOT)/support/specAn/application/common/build/smake/paths.mk
endif

include $(TARGET_SUPPORT_ROOT)/target_support.mk


export HOSTAPP_LINK_SCRIPT=$(SA_SUPPORT_ROOT)/loaders/testApp/source/meta/SpecAn_$(UCCP).ldscript

IMGS+=SpecAn_$(UCCP).img

APPS=$(UNIT_DIR)/../../application/testApp \
    $(UNIT_DIR)/../../application/testApp/@MakefileGRAM

include $(SMAKE_ROOT)/smake.mk
