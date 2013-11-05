# Makefile fragment to set up variables defining module paths.
# Two directory structures are supported: the CVS structure (default) and the Perforce structure
# (if P4 defined)
# This fragment is actually included from the loader makefile, but is located in the common
# application area as it is common to all applications.

# The calling makefile defines CODELINE_ROOT, this is an absolute path pointing to the root of the
# codeline in P4 structure (i.e DEV, MAIN etc).  In the CVS structure this points to the mobileTV
# directory

ifdef P4
export IP_ROOT=$(CODELINE_ROOT)/ip/software
export SA_SUPPORT_ROOT=$(CODELINE_ROOT)/support
export LOCAL_DRIVERS_ROOT=$(CODELINE_ROOT)/ip/software/uccDrivers
export IMPORTED_DRIVERS_ROOT=$(CODELINE_ROOT)/imports/common/uccDrivers
export TVCORE_ROOT=$(CODELINE_ROOT)/imports/common/uccTVAppShell/software
export PFM_SETUP_ROOT=$(CODELINE_ROOT)/support
export TARGET_SUPPORT_ROOT=$(CODELINE_ROOT)/support/platform_setup
export MATHS_FUNCS_ROOT=$(CODELINE_ROOT)/imports/common
export AGC_ROOT=$(CODELINE_ROOT)/imports/common
export MCPOS_ROOT=$(UCC_INST_ROOT)/src
export UCC_FRAMEWOK_ROOT=$(CODELINE_ROOT)/imports/common/uccTVAppShell/software
export PLT_SUPPORT_ROOT=$(CODELINE_ROOT)/imports/platform
export TUNERS_ROOT=$(CODELINE_ROOT)/imports/common/tuners
export GENERIC_TUNER_ROOT=$(CODELINE_ROOT)/imports/common/uccTVAppShell/software/tuners
export HOSTAPP_ROOT=$(CODELINE_ROOT)/imports/common
export IRQ_GEN_ROOT=$(UCC_INST_ROOT)/src
else
export IP_ROOT=$(CODELINE_ROOT)/specAn/software
export SA_SUPPORT_ROOT=$(CODELINE_ROOT)/support/specAn
export LOCAL_DRIVERS_ROOT=$(CODELINE_ROOT)/common/uccDrivers
export IMPORTED_DRIVERS_ROOT=$(CODELINE_ROOT)/common/uccDrivers
export TVCORE_ROOT=$(CODELINE_ROOT)/common
export PFM_SETUP_ROOT=$(CODELINE_ROOT)/support/specAn/application
export TARGET_SUPPORT_ROOT=$(CODELINE_ROOT)/support/specAn/application/platform_setup
export MATHS_FUNCS_ROOT=$(CODELINE_ROOT)/common
export AGC_ROOT=$(CODELINE_ROOT)/common
export MCPOS_ROOT=$(CODELINE_ROOT)/common
export UCC_FRAMEWOK_ROOT=$(CODELINE_ROOT)/support
export PLT_SUPPORT_ROOT=$(CODELINE_ROOT)/support/platform
export TUNERS_ROOT=$(CODELINE_ROOT)/support/common/tuners
export GENERIC_TUNER_ROOT=$(CODELINE_ROOT)/support/common/tuners
export HOSTAPP_ROOT=$(CODELINE_ROOT)/support/specAn/application/common
export IRQ_GEN_ROOT=$(CODELINE_ROOT)/common/uccDrivers
endif

# UCC_DRIVERS_ROOT is used by uccDrivers.mk at uccDrivers top level
export UCC_DRIVERS_ROOT=$(IMPORTED_DRIVERS_ROOT)
