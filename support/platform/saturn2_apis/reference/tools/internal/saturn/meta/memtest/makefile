############################################################################################
#
# Makefile for creating memtest elf and ldr for Comet META.
#
# The command line options are:
#
#    make help               - display help information
#    make clean              - prepare for full rebuild
#    make                    - build using defaults (ADDR=low is the default)
#    make ADDR=low           - build for loading to low system SRAM memory (default)
#    make ADDR=high          - build for loading to high system SRAM memory
#
############################################################################################

BINDIR = $(METAG_INST_ROOT)/metag-local/bin/
LIBS = $(METAG_INST_ROOT)/metag-local/lib/
LIBS2 = $(METAG_INST_ROOT)/build_$(BUILD_VER)/gmake/lib
CC = $(BINDIR)gcc
LD = $(BINDIR)ld
LDLK = $(BINDIR)ldlk

CFLAGS = -save-temps -DMETAC_2_1
CFLAGS += -I$(METAG_INST_ROOT)/metag-local/include/metag
##CFLAGS += -DMEMTEST_START=0x82883000
##CFLAGS += -DMEMTEST_RANGE=0x00001000
##CFLAGS += -DMEMTEST_CHUNKS=2
##CFLAGS += -DMEMTEST_BYTES=0x00000800
##CFLAGS += -DMEMTEST_WAITTIME=0
##CFLAGS += -DMEMTEST_REPEAT=1
CFLAGS += -g

LDPREFLAGS = -r $(LIBS)crt1.o -L$(LIBS2) -L$(LIBS)

#
## Link at 0xE0200000 for low-memory resident version and 0xE02580000 for high-memory resident version,
## allowing 16kbytes (0x4000) for both code and data (plus stack) sections.
##
## Code and data (plus stack) sections SHOULD be both less than 16kbytes - but check in case it's grown!
# 

ifdef ADDR
  ifeq ($(ADDR), low)
    HILOMSG = "Building for loading at the bottom of system SRAM memory."
    HILO = low
    LINK_ADDR = -Ttext 0x80900000 -Tdata 0x82884000
    CFLAGS += -DADDR_LOW
  else
    ifeq ($(ADDR), high)
      HILOMSG = "Building for loading at the top of system SRAM memory."
      HILO = high
      LINK_ADDR = -Ttext 0x80990000 -Tdata 0x828CC000
      CFLAGS += -DADDR_HIGH
    else
      $(error Invalid ADDR option specified, type "make help" for instructions )
    endif
  endif
else
  ## Default is "low"
  HILOMSG = "Defaulting to building for loading at the bottom of system SRAM memory."
  HILO = low
  LINK_ADDR = -Ttext 0x80900000 -Tdata 0x82884000
  CFLAGS += -DADDR_LOW
endif

IMG_CMD = -Tmemtest_$(HILO).img
LDFLAGS = -l low -l gcc -l c -l mgloss $(METAG_INST_ROOT)/metag-local/lib/crtn.o $(LINK_ADDR)

SOURCES=main.c memtest.c tests.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=memtest

all: $(SOURCES) $(EXECUTABLE).elf $(EXECUTABLE).ldr

$(EXECUTABLE).ldr: $(OBJECTS)
	@echo ""
	@echo "Creating $(HILO)_$(EXECUTABLE).ldr file ...."
	@echo ""
	$(LDLK) --verbose -g $(IMG_CMD) -o$(HILO)_$@
	
$(EXECUTABLE).elf: $(OBJECTS)
	@echo ""
	@echo $(HILOMSG)
	@echo ""
	@echo "Creating $(HILO)_$(EXECUTABLE).elf file ...."
	@echo ""
	$(LD) $(LDPREFLAGS) -o $(HILO)_$@ $(OBJECTS) $(LDFLAGS)
	
%.o: %.c
	@echo ""
	@echo "Compiling source files ...."
	@echo ""
	$(CC) -c $(CFLAGS) $< -o $@
	
clean:
	@echo ""
	@echo "Preparing for full rebuild."
	@echo "Deleting *.i *.o *.ldr *.elf *.s ...."
	@echo ""
	rm -f *.i *.o *.ldr *.elf *.s *.js *.py
	
help:
	@echo ""
	@echo "This Comet META memory test can be built to load either at the bottom of system SRAM memory and test"
	@echo "the upper system SRAM memory region (352kb) and various Comet internal and external memory types via the"
	@echo "Comet physical memory space (0xE0200000 internal and B0000000 external), or built to load at the top of"
	@echo "system SRAM and test the lower core memory region (again, 352kb) and the same internal and external"
	@echo "memory as previously described."
	@echo ""
	@echo "Examples:"
	@echo ""
	@echo "  make               - default make, builds for low memory loading (equivalent to make ADDR=low - see below)"
	@echo "  make ADDR=low      - builds for low memory loading, creates low_memtest.elf and low_memtest.dnl"
	@echo "  make ADDR=high     - builds for high memory loading, creates high_memtest.elf and high_memtest.dnl"
	@echo ""
	@echo "  make clean         - prepare for full rebuild"
	@echo ""
	
