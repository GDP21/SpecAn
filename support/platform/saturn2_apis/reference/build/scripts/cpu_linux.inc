###############################################
###############################################
##
##	Linux Target
##
##

CYGWIN_USR = /usr

###############################################
###############################################
##
##	Create some more variables
##

## Silence warnings (mostly from systemC) about deprecated c++ headers.
CPPFLAGS += -Wno-deprecated

## Binding global symbol/references locally into the binaries, so that each binary (.exe and .so) can have independent instance of so and meos abstraction layer
LDFLAGS += -Wl,-Bsymbolic

ifdef THREAD
	LINUX_BUILD = 1
	INCLUDES += -I$(CYGWIN_USR)/include/
	INCLUDES += -I$(IMG_WORKROOT)/include/Linux/
	ifeq ($(RELEASE), debug)
		CFLAGS += -DCYGWIN -D__linux__ -Wno-long-long -O0 -g $(INCLUDES)
	else
		ifeq ($(RELEASE), test)
			CFLAGS += -DCYGWIN -D__linux__ -Wno-long-long -O2 -g $(INCLUDES)
		else
			CFLAGS += -DCYGWIN -D__linux__ -Wno-long-long -O2 $(INCLUDES)
		endif
	endif
	ifeq ($(FORCE_32BIT), true)
		CFLAGS += -m32
		LDFLAGS += -m32
	endif
	ifeq ($(FORCE_64BIT), true)
		CFLAGS += -m64
		LDFLAGS += -m64
	endif
endif

CRT1 =
CRT2 =

## If no libs required
ifdef IMG_META_NOLIB
	CRT1 =
	CRT2 =
	LDFLAGS = -Map $@.map
endif

###############################################
###############################################
##
##	Path variables
##
BINDIR = $(CYGWIN_USR)/bin/
BINDIR2 = $(CYGWIN_USR)/bin/

###############################################
###############################################
##
##	Program variables
##
CC = $(BINDIR)gcc
AS = $(BINDIR)as
AR = $(BINDIR2)ar
RANLIB = $(BINDIR2)ranlib
LD = $(BINDIR)g++
##XSLT = testXSLT
CP = cp
