###############################################
###############################################
##
##	SATURN ES1 HW Target
##
##

CFLAGS += -D__IMG_HW_ES1__

## These pick up the IMG platform definition from either the top level makefile, or the command line,
## and can be used in the code along with __IMG_PROJ_XXX__ to compile in or out platform specific code.

ifeq ($(IMG_PLATFORM),hw)
  CFLAGS += -D__IMG_HW_SI__
else ifeq ($(IMG_PLATFORM),emu)
  CFLAGS += -D__IMG_HW_EMU__
else ifeq ($(IMG_PLATFORM),fpga)
  CFLAGS += -D__IMG_HW_FPGA__
endif
