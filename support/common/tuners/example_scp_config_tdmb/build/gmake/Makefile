#__TITLE__(Makefile for example SCP configuration)*/

#################################################################
#
# FILE NAME:     $Source: /cvs/mobileTV/support/common/tuners/example_scp_config_tdmb/build/gmake/Makefile,v $
#
# PROJECT:       Mobile TV
#
# AUTHOR:        Ensigma
#
# DESCRIPTION:   Makefile for building the example SCP configuration
#
# NOTICE:        Copyright (C) 2008, Imagination Technologies Ltd.
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
SCP_CONFIG_ID=$(TARGET)_example_scp_config_tdmb$(OBJMOD)
SCP_CONFIG_LIB=$(METAGS_LIB)/lib$(SCP_CONFIG_ID).a

#
# names for published "headers" and documents
#
SCP_CONFIG_HDR=$(SRCDIR)/example_scp_config_tdmb.h 

#
# what we build and install externally
#
EXTERNALS=$(SCP_CONFIG_LIB)
EXTHDRS=$(SCP_CONFIG_HDR)
EXTDOCS= 

#
# OBJECT FILE LIST
#
OBJECTS=$(OBJDIR)/example_scp_config_tdmb$(OBJMOD).o

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
$(SCP_CONFIG_LIB) : $(OBJECTS)
	$(CMDECHO3)if [ ! -d $(METAGS_LIB) ] ; then \
		$(RAWECHO2) "Making $(METAGS_LIB)"; \
		mkdir $(METAGS_LIB); fi
	$(CMDECHO3)for obj in $(OBJECTS); do \
		$(RAWECHO2) "Adding $$obj to $(@F) ..." ;\
		$(LMETALIBU) $@ $$obj; done
	$(LOGECHO) "Re-indexing $(@F) ..."
	$(CMDECHO2)$(LMETALIBR) $@

#clobber:
#	rm -f $(OBJECTS) $(SCP_CONFIG_LIB)


#
# local module help target
#
subhelp:
	@echo ""
	@echo "This builds and/or tests the building of the example SCP configuration for T-DMB"
	@echo "Targets:"
	@echo "    default:"
	@echo "    all:        Builds (public) lib$(SCP_CONFIG_ID).a plus headers"
	@echo ""

#
# Additional dependency and build rules
#
