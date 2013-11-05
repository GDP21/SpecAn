###############################################
###############################################
##
##	Meta CPU Target
##
##

ifndef METAG_INST_ROOT
 $(error ERROR: METAG_INST_ROOT not defined. e.g. /img/usr/metag/2.4.0b2)
endif

###############################################
###############################################
##
##	Create some more variables
##
export METAG_INST_ROOT_LD = $(shell cygpath -am $(METAG_INST_ROOT))

ifdef THREAD
  INCLUDES += -I$(IMG_WORKROOT)/include/meta
  ifeq ($(RELEASE), debug)
    CFLAGS += -D__FAF__="" -DMETAG -Wall -O0 -g $(INCLUDES)
  else
    ifeq ($(RELEASE), test)
      CFLAGS += -D__FAF__="" -DMETAG -Wall -O2 -g $(INCLUDES)
    else
      CFLAGS += -D__FAF__="" -DMETAG -Wall -O2 $(INCLUDES)
    endif
  endif
  CFLAGS += -mmodel=large
endif

ifdef GEN_MAPFILE
	MAPFILE_OPT=-Map $@.map:9 -MM
endif

ifdef USES_SHARED_LIB
  ifeq ($(THREAD),threads)
    CRT1 = $(METAG_INST_ROOT_LD)/metag-local/lib/crt2.o
    CRT2 = $(METAG_INST_ROOT_LD)/metag-local/lib/crtn.o
    LDFLAGS = -r --syms-in $@.syms --shared-static $(MAPFILE_OPT)
 else
    CRT1 = $(METAG_INST_ROOT_LD)/metag-local/lib/crt1.o $(OBJECTS)
    CRT2 = -L$(METAG_INST_ROOT_LD)/lib/gcc-lib/metag-local/2.95.2 \
           -L$(METAG_INST_ROOT_LD)/metag-local/lib \
           $(METAG_INST_ROOT_LD)/metag-local/lib/crtn.o
    LDFLAGS = -r --syms-out $@.syms $(MAPFILE_OPT)
  endif
else
  CRT1 = $(METAG_INST_ROOT_LD)/metag-local/lib/crt1.o
  CRT2 = -L$(METAG_INST_ROOT_LD)/lib/gcc-lib/metag-local/2.95.2 \
         -L$(METAG_INST_ROOT_LD)/metag-local/lib \
         -lc -llow -lgcc -lmgloss -lm\
         $(METAG_INST_ROOT_LD)/metag-local/lib/crtn.o
  LDFLAGS = -r $(MAPFILE_OPT)
endif
LDFLAGS += --memmodel large

## over-ride for global elf
ifeq ($(THREAD),threadg)
  CRT1 =
  CRT2 =
  LDFLAGS = -r $(MAPFILE_OPT)
endif

## If no libs required
ifdef IMG_META_NOLIB
  CRT1 =
  CRT2 =
  LDFLAGS = -r $(MAPFILE_OPT)
endif

###############################################
###############################################
##
##	Path variables
##
ifeq ($(OS),Windows_NT)
	BINDIR = $(METAG_INST_ROOT)/metag-local/bin/
	BINDIR2 = $(METAG_INST_ROOT)/bin/metag-local-
	BINDIR_LD = $(METAG_INST_ROOT_LD)/metag-local/bin/
	BINDIR_LD2 = $(METAG_INST_ROOT_LD)/bin/metag-local-
else
	BINDIR = $(METAG_INST_ROOT)/metag-local/bin/
	BINDIR2 = $(METAG_INST_ROOT)/metag-local/bin/
	BINDIR_LD = $(METAG_INST_ROOT)/metag-local/bin/
	BINDIR_LD2 = $(METAG_INST_ROOT)/metag-local/bin/
endif

###############################################
###############################################
##
##	Program variables
##
CC = $(BINDIR)gcc
AS = $(BINDIR2)as
AR = $(BINDIR2)ar
RANLIB = $(BINDIR2)ranlib
LD = $(BINDIR_LD2)ld
LDLK = $(BINDIR)ldlk
XSLT = testXSLT
CP = cp
