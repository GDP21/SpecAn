###############################################
###############################################
##
## This makefile exists to build a Comet library.  To
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
##  This doesn't actually exist.  It is here to 
##  satisfy the requiremetns of the build system

SRCDIR := code/

###############################################
##
##	TARGETS - list the final target(s) of this
##			makefile.
##			Valid targets: ?.a ?.h ?.elf ?.ldr
##
##  These are not real targets - ie they cannot
##  actualy be built, but are needed to keep the
##  build system happy.
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
