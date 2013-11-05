###############################################
###############################################
##
##	Cygwin Target
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

ifdef THREAD
  INCLUDES += -I$(IMG_WORKROOT)/include/win32/
  ifeq ($(RELEASE), debug)
    CFLAGS += -DWIN32 -D_DEBUG -DWINVER=0x0501 -D_X86_
    CFLAGS += -DCYGWIN -Wno-long-long -O0 -g $(INCLUDES)
  else
    ifeq ($(RELEASE), test)
      CFLAGS += -DWIN32
      CFLAGS += -DCYGWIN -Wno-long-long -O2 -g $(INCLUDES)
    else
      CFLAGS += -DWIN32
      CFLAGS += -DCYGWIN -Wno-long-long -O2 $(INCLUDES)
    endif
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
