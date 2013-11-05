#
# Target support include file
#
# Include this file in your app and loader makefiles to pull in standard
# target support setup.
#
# You need to setup MOBILETV_ROOT to point at the mobileTV root directory
#

#
# Check CODELINE_ROOT is defined
#
#ifndef CODELINE_ROOT
#$(error CODELINE_ROOT undefined)
#endif

#$(warning target_support $(TARGET))

#
# Target support root directory.
# Board fragments may also use this.
#
TARGET_SUPPORT_ROOT:=$(CODELINE_ROOT)/support/specAn/application/platform_setup

#############################################################################

#
# Paths
#
TARGETS_PATH:=$(TARGET_SUPPORT_ROOT)/targets
SOCS_PATH:=$(TARGET_SUPPORT_ROOT)/socs
TUNERS_PATH:=$(TARGET_SUPPORT_ROOT)/tuners
UCCPS_PATH:=$(TARGET_SUPPORT_ROOT)/uccps

#############################################################################

#
# Set default target
#
ifeq ($(TARGET),NONE)
ALL_TARGET_FRAGS:=$(wildcard $(TARGETS_PATH)/*/*.mk)
ifeq ($(words $(ALL_TARGET_FRAGS)),1)
GLOBAL_TARGET:=$(shell echo $(basename $(notdir $(ALL_TARGET_FRAGS))) | awk '{print toupper($$0)}')
else
GLOBAL_TARGET=EMU
endif
endif

#############################################################################

#
# Setup target, tuner, SOC, UCCP and OS.
# The order is important here and must be as above.
#

#
# Target fragments
#
LC_TARGET:=$(shell echo $(TARGET) | awk '{print tolower($$0)}')
TARGET_FRAG:=$(wildcard $(TARGETS_PATH)/$(LC_TARGET)/$(LC_TARGET).mk)
ifneq (,$(TARGET_FRAG))
include $(TARGET_FRAG)
else
$(error A target fragment for TARGET=$(TARGET) does not exist)
endif

#
# Target phase now done
#
export TARGET_DONE=1

#
# Set default RF.
# Must do this after target fragments (so as not to affect what they setup)
# but before tuner fragments (because RF must be valid by then).
#
RF?=DUMMY

#
# Tuner fragments
#
LC_RF:=$(shell echo $(RF) | awk '{print tolower($$0)}')
TUNER_FRAG:=$(wildcard $(TUNERS_PATH)/$(LC_RF)/tuner_$(LC_RF).mk)
ifneq (,$(TUNER_FRAG))
include $(TUNER_FRAG)
else
$(error A tuner fragment for RF=$(RF) does not exist)
endif

#$(warning SOC $(SOC))

#
# SOC fragments
#
LC_SOC:=$(shell echo $(SOC) | awk '{print tolower($$0)}')
SOC_FRAG:=$(wildcard $(SOCS_PATH)/$(LC_SOC)/soc_$(LC_SOC).mk)
ifneq (,$(SOC_FRAG))
include $(SOC_FRAG)
else
$(error An SOC fragment for SOC=$(SOC) does not exist)
endif

#
# Set default UCCP
#
ifeq ($(UCCP),NONE)
ALL_UCCP_FRAGS:=$(wildcard $(UCCPS_PATH)/*/*.mk)
ifeq ($(words $(ALL_UCCP_FRAGS)),1)
GLOBAL_UCCP:=$(subst uccp,,$(basename $(notdir $(ALL_UCCP_FRAGS))))
else
$(error UCCP not defined)
endif
endif

#
# UCCP
#
UCCP_FRAG:=$(wildcard $(UCCPS_PATH)/uccp$(UCCP)/uccp$(UCCP).mk)
ifneq (,$(UCCP_FRAG))
include $(UCCP_FRAG)
else
$(error A UCCP fragment for UCCP=$(UCCP) does not exist)
endif

#
# Tuner support
#
ifdef NEEDS_TUNER_SUPPORT
MODS+=$(CODELINE_ROOT)/support/common/tuner_support
# tuner_support uses log10f
UNIT_IMPORTS+=m
endif

#
# OS setup
#
ifdef NEEDS_MEOS
# Require a fully setup MeOS environment
#GLOBAL_OPERATING_SYSTEM=MEOS
GLOBAL_DEFINES+=__MEOS__
else
ifdef LINK_MEOS
# Just link with MeOS
#UNIT_IMPORTS+=METAMeOS
endif
endif

#
# Export everything needed by the platform setup module
#
ifdef SOC
export SOC
endif

ifdef RF
export RF
endif

ifdef IF
export IF
GLOBAL_DEFINES+=SCP_$(IF)
endif

ifdef STC
export STC
endif

ifdef PLT_C_SRC
export PLT_C_SRC
endif

ifdef PLT_TARGET_SRCDIRS
export PLT_TARGET_SRCDIRS
endif

ifdef PLT_TUNER_SRCDIRS
export PLT_TUNER_SRCDIRS
endif

ifdef PLT_SOC_SRCDIRS
export PLT_SOC_SRCDIRS
endif

ifdef PLT_UCCP_SRCDIRS
export PLT_UCCP_SRCDIRS
endif

#############################################################################
