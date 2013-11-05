###############################################
###############################################
##
##	MTX CPU Target
##
##

ifndef METAG_INST_ROOT
 $(error ERROR: METAG_INST_ROOT not defined. e.g. /img/usr/mtxg/2.3.2b4)
endif

###############################################
###############################################
##
##	Create some more variables
##
export METAG_INST_ROOT_LD = $(shell cygpath -am $(METAG_INST_ROOT))

ifdef THREAD
  INCLUDES += -I$(IMG_WORKROOT)/include/$(IMG_CPU)
  INCLUDES += -I$(METAG_INST_ROOT)/mtxg-local/include/metag
  ifeq ($(RELEASE), debug)
    CFLAGS += -DMTXG -Wall -O0 -g $(INCLUDES)
  else
    ifeq ($(RELEASE), test)
      CFLAGS += -DMTXG -Wall -O2 -g $(INCLUDES)
    else
      CFLAGS += -DMTXG -Wall -O2 $(INCLUDES)
    endif
  endif
endif

LDFLAGS = -Map $@.map

## If no libs required
ifdef IMG_META_NOLIB
  CRT1 =
  CRT2 =
else

  ifdef USE_CUSTOM_TOPAZSC_FLAGS

    NM = $(BINDIR)nm

    CRT1 = $(METAG_INST_ROOT_LD)/mtxg-local/lib/crt1.o $(METAG_INST_ROOT_LD)/mtxg-local/lib/crt2.o

    ifeq ($(RELEASE), release)
      CRT2 = -L$(METAG_INST_ROOT_LD)/mtxg-local/lib -llow -lmgloss $(METAG_INST_ROOT_LD)/mtxg-local/lib/crtn.o $(METAG_INST_ROOT_LD)/mtxg-local/lib/libgcc.a
    else
      CRT2 = -L$(METAG_INST_ROOT_LD)/mtxg-local/lib -lc -llow -lgcc -lmgloss -lm -llow $(METAG_INST_ROOT_LD)/mtxg-local/lib/crtn.o $(METAG_INST_ROOT_LD)/mtxg-local/lib/libgcc.a
    endif

    ifeq ($(RELEASE), release)
      LDFLAGS += -Ur -Us
    else
      LDFLAGS += -Ur -Uoff
    endif

  else
    CRT1 = $(METAG_INST_ROOT_LD)/mtxg-local/lib/crt1.o $(METAG_INST_ROOT_LD)/mtxg-local/lib/crt2.o

    ifeq ($(RELEASE), release)
      CRT2_LIBS = -L$(METAG_INST_ROOT_LD)/mtxg-local/lib -llow -lmgloss
      CRT2 = $(CRT2_LIBS) $(METAG_INST_ROOT_LD)/mtxg-local/lib/crtn.o
    else
      CRT2_LIBS = -L$(METAG_INST_ROOT_LD)/mtxg-local/lib -lc -llow -lgcc -lmgloss -lm -llow
      CRT2 = $(CRT2_LIBS) $(METAG_INST_ROOT_LD)/mtxg-local/lib/crtn.o
    endif

    ifeq ($(RELEASE), release)
      LDFLAGS += -Us
    else
      LDFLAGS += -Uoff
    endif

  endif

  ifdef NO_CRTN
    CRT2 =
  endif

endif

###############################################
###############################################
##
##	Path variables
##
BINDIR = $(METAG_INST_ROOT)/mtxg-local/bin/
BINDIR2 = $(METAG_INST_ROOT)/bin/metag-local-

###############################################
###############################################
##
##	Program variables
##
CC = $(BINDIR)gcc
AS = $(BINDIR)as
AR = $(BINDIR2)ar
RANLIB = $(BINDIR2)ranlib
LD = $(BINDIR)ld
##XSLT = testXSLT
CP = cp
