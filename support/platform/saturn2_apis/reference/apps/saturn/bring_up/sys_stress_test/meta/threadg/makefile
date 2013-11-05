##############################################
##
##	THREAD - Only ONE thread, this is used to
##           separate object files etc.
##
export THREAD := threadg


###############################################
##
##	Build Options
##  =============
##
##	OS:		{meos_Vx.xx|nucleus_Vx.xx|uiplus_Vx.xx}
##
##
export IMG_OS := meos_preinstalled
export TAL_LIGHT := TRUE

###############################################
##
##	Include generic makefile (first half)
##
include $(MAK_DIR)/make.inc


###############################################
##
##	SRC - list all 'C' source files
##

SRCDIR := code/

SRC := 	gram_buffers.c


###############################################
##
##	TARGETS - list the final target(s) of this
##			makefile.
##			Valid targets: ?.a ?.h ?.elf ?.ldr
##
BUILDDIR := ../$(RELEASE)/
TARGETS=$(BUILDDIR)$(THREAD).elf


###############################################
##
##	LIBS - list any extra libraries required by the target
##
LIBS +=


###############################################
###############################################

###############################################
##
##	Include generic makefile (second half)
##
include $(MAK_DIR)/rules.inc

