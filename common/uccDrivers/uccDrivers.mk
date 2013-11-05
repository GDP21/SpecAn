#
# smake app include file for the UCC drivers.
#
# Just include this file in your app to pull in the configuration directories
# for the UCC drivers.
#
# The path to the uccDrivers directory needs to be specified in
# UCC_DRIVERS_ROOT. This can either be relative or absolute.
#

#############################################################################

#
# Only include once
#
ifndef UCC_DRIVERS_INCLUDED
export UCC_DRIVERS_INCLUDED=1

#############################################################################

#
# Check that UCC_DRIVERS_ROOT has been defined
#
ifndef UCC_DRIVERS_ROOT
$(error UCC_DRIVERS_ROOT not defined. This should point at the uccDrivers directory)
endif

#
# Get an absolute version of UCC_DRIVERS_ROOT
#
UCC_DRIVERS_ABS_ROOT:=$(shell cd $(UCC_DRIVERS_ROOT);pwd)

#############################################################################

#
# Check that UCCP is defined and set plausibly.
# UCCP could be an smake global value variable, but there is nothing in this
# fragment that mandates this.
#
ifndef UCCP
$(error UCCP not defined)
endif
ifeq ($(UCCP),NONE)
$(error UCCP has not been set)
endif

#############################################################################

#
# Include all the fragments present
#
UCC_DRIVERS_FRAGS:=$(wildcard $(UCC_DRIVERS_ABS_ROOT)/*/*Driver.mk)
ifdef UCC_DRIVERS_FRAGS
include $(UCC_DRIVERS_FRAGS)
endif

#############################################################################

#
# Add the top-level path onto the directories listed in the fragments
#
export UCC_DRIVERS_DIRS:=$(addprefix $(UCC_DRIVERS_ABS_ROOT)/,$(UCC_DRIVERS_DIRS))

#############################################################################

endif # !UCC_DRIVERS_INCLUDED

#############################################################################
