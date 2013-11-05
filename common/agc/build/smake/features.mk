UNIT_NAME_SUFFIX=_UCC$(UCC)

# Detect any build variants and create UNIT_NAME_SUFFIX to reflect it
#

ifeq ($(findstring TDMB, $(DEFINES)), TDMB)
# Being built for TDMB so name unit accordingly.
UNIT_NAME_SUFFIX:=_TDMB$(UNIT_NAME_SUFFIX)
endif