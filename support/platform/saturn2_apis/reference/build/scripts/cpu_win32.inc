###############################################
###############################################
##
##	Win32 Target
##
##

###############################################
###############################################
##
##	Create some more variables
##

ifneq (,$(strip $(SIM_ROOT)))
	SIM_ROOT := $(shell cygpath -u '$(SIM_ROOT)')
endif

ifdef THREAD
	MSDEV = 1

	####################################################################
	##
	## Set GCCFLAGS that will be used by GCC for calculating dependenies
	##
	ifdef WIN32_INCLUDE3
	    INCLUDE3=$(WIN32_INCLUDE3)
	else
	    empty:=
	    space:= $(empty) $(empty)
	    semicolon:=;
	    INCLUDE1='$(subst $(semicolon),' ',$(INCLUDE))'
	    #$(warning INCLUDE1=$(INCLUDE1))
	    INCLUDE2 := $(shell cygpath -u $(INCLUDE1))
	    #$(warning INCLUDE2=$(INCLUDE2))
	    INCLUDE3=-I'$(subst $(space)/,' -I/',$(INCLUDE2))'
	    #$(warning INCLUDE3=$(INCLUDE3))
	    export WIN32_INCLUDE3=$(INCLUDE3)
	endif

	GCCFLAGS += -D_WIN32 -D_M_IX86 -D__STDC__=0 $(INCLUDE3)


	INCLUDES += -I$(IMG_WORKROOT)/include/win32/ 
	CFLAGS += -DWIN32 -D_WINDOWS -D_MBCS 
	CLFLAGS += -Gm -EHsc -RTC1 -GS -W3 -c -ZI -GR

	ifeq ($(IMG_APP_TYPE), console)
		CFLAGS += -D_LIB -D_CONSOLE
		ifeq ($(RELEASE), debug)
			CLFLAGS += -MTd
		else
			ifeq ($(RELEASE), test)
				CLFLAGS += -MTd
			else
				CLFLAGS += -MT
			endif
		endif
		LDFLAGS += /INCREMENTAL /NOLOGO /DEBUG /SUBSYSTEM:CONSOLE /MACHINE:X86 
		CRT1 = winmm.lib Ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib
		CRT2 =
	else
		ifeq ($(IMG_APP_TYPE), mfcdll)
			CFLAGS += -D_WINDLL -D_AFXDLL -D_USRDLL
			CLFLAGS += -MDd 
			GCCFLAGS += -D_DLL -D_MT
			LDFLAGS += /INCREMENTAL /NOLOGO /DLL /DEBUG /SUBSYSTEM:WINDOWS /MACHINE:X86  
			CRT1 = winmm.lib
			CRT2 =
		else 
			ifeq ($(IMG_APP_TYPE), dll)
				CFLAGS += -D_WINDLL -D_USRDLL
				CLFLAGS += -MDd 
				GCCFLAGS += -D_DLL -D_MT
				LDFLAGS += /INCREMENTAL /NOLOGO /DLL /DEBUG /SUBSYSTEM:WINDOWS /MACHINE:X86  
				CRT1 = winmm.lib Ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib
				CRT2 =
			else 		
				$(error ** ERROR: Build type not recognised )
			endif
		endif
	endif

	ifeq ($(RELEASE), debug)
		CFLAGS += -D_DEBUG 
	else
		ifeq ($(RELEASE), test)
			CFLAGS += -D_TEST 
		else
			CFLAGS += -D_RELEASE 
		endif
	endif
	CFLAGS += $(INCLUDES)
endif

## If no libs required
ifdef IMG_META_NOLIB
	CRT1 =
	CRT2 =
	LDFLAGS = 
endif

CFLAGS+= -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS
###############################################
###############################################
##
##	Path variables
##
BINDIR = $(CYGWIN_USR)/bin/

###############################################
###############################################
##
##	Program variables
##
CC = $(BINDIR)gcc
CL_EXE = cl.exe
LIB_EXE = lib.exe
LINK_EXE = link.exe
