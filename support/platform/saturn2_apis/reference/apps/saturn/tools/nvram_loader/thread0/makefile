###############################################
###############################################
##
##
## This makefile exists to build a test application and also
## keep the build system happy (having a "normal" app build
## a .elf on thread0 causes the sub libs stuff to work so
## things like "make libclean" work currectly).
##
###############################################
###############################################

###############################################
##
##	THREAD - Only ONE thread, this is used to
##           separate object files etc.
##
export THREAD := thread0

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

SRC := nvram_loader.c


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

