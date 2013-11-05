#__TITLE__(Makefile for MSI tuner driver)*/

#################################################################
#
# FILE NAME:     $Source: /cvs/mobileTV/support/common/tuners/msi_tuner/build/gmake/Makefile,v $
#
# PROJECT:       Mobile TV
#
# AUTHOR:        Ensigma
#
# DESCRIPTION:   Makefile for building the MSI tuner driver
#
# NOTICE:        Copyright (C) 2007, Imagination Technologies Ltd.
#                This software is supplied under a licence agreement.
#                It must not be copied or distributed except under the
#                terms of that agreement.
#
################################################################

#
# default test tree root - uses generic meta module build
#
ifndef TESTROOT
TESTROOT=$(METAG_INST_ROOT)/build$(_GMAKE_VERSION)/metagtest
endif

#
# standard make system - configure options
#
_GMAKE_OPTIONS=1
include $(TESTROOT)/gmake/testsdef.mk
_GMAKE_OPTIONS=

# uncomment the following to force tools to build for META 1.2
# Commented out - use default processor for toolkit
#METAC_DEFAULT_AS=METAC_1_2
#METAC_DEFAULT=1.2

#
# default optimisation levels
#
ifndef METACOPT
    ifndef DEBUG
        METACOPT=2
    else
        METACOPT=0
    endif
endif

#
# Extra C options 
#
COPTEXTRA=-I$(METAGS_INC) -I../../../../../../include

#
# source file location
#
SRCDIR=../../source/generic

#
# names for the public lists module library files
#
TUNER_ID=$(TARGET)_msi_tuner$(OBJMOD)
TUNER_LIB=$(METAGS_LIB)/lib$(TUNER_ID).a

#
# names for published "headers" and documents
#
TUNER_HDR=$(SRCDIR)/msi_tuner.h 

#
# what we build and install externally
#
EXTERNALS=$(TUNER_LIB)
EXTHDRS=$(TUNER_HDR)
EXTDOCS= 

#
# OBJECT FILE LIST
#
OBJECTS=$(OBJDIR)/msi_tuner$(OBJMOD).o

ifdef MSI_SPIM
OBJECTS+=$(OBJDIR)/msi_spim$(OBJMOD).o
COPTEXTRA+=-DMSI_SPIM
endif

ifdef MSI_SCBM
OBJECTS+=$(OBJDIR)/msi_scbm$(OBJMOD).o
COPTEXTRA+=-DMSI_SCBM
endif

ifdef MSI_001
COPTEXTRA+=-DMSI_001
endif

ifdef MSI_002
OBJECTS+=$(OBJDIR)/msi_002$(OBJMOD).o
COPTEXTRA+=-DMSI_002
endif

#
# common build technology
#
include $(TESTROOT)/gmake/testsdef.mk

ifeq ($(filter clobber,$(MAKECMDGOALS)),)
-include $(OBJECTS:.o=.d)
endif

#
# default target - builds libraries and publishes headers
#
all: exthdr $(EXTERNALS)

#
# how to build the library targets
#
$(TUNER_LIB) : $(OBJECTS)
	$(CMDECHO3)if [ ! -d $(METAGS_LIB) ] ; then \
		$(RAWECHO2) "Making $(METAGS_LIB)"; \
		mkdir $(METAGS_LIB); fi
	$(CMDECHO3)for obj in $(OBJECTS); do \
		$(RAWECHO2) "Adding $$obj to $(@F) ..." ;\
		$(LMETALIBU) $@ $$obj; done
	$(LOGECHO) "Re-indexing $(@F) ..."
	$(CMDECHO2)$(LMETALIBR) $@

#
# local module help target
#
subhelp:
	@echo ""
	@echo "This builds and/or tests the building of the MSI tuner driver"
	@echo "Targets:"
	@echo "    default:"
	@echo "    all:        Builds (public) lib$(TUNER_ID).a plus headers"
	@echo ""

#
# Additional dependency and build rules
#
