###############################################
###############################################
##
##	This file is included at the
##		start of a makefile
##
###############################################
###############################################

ifndef REQ_MAKE_VERSION
  export REQ_MAKE_VERSION := 3.81
endif

ifdef VERBOSE
  SILENT=
else
  SILENT=-s
endif


###############################################
###############################################
##
##	Check the version of make
##

ifndef LATEST_MAKE_VERSION
  export LATEST_MAKE_VERSION := $(firstword $(sort $(MAKE_VERSION) $(REQ_MAKE_VERSION)))
  ifneq ($(LATEST_MAKE_VERSION), $(REQ_MAKE_VERSION))
    $(error ** ERROR: Requires Make v$(REQ_MAKE_VERSION) or above )
  endif
endif


###############################################
###############################################
##
##	Clear G_CFLAGS
##
G_CFLAGS=

###############################################
###############################################
##
##	Define the project make directory
##
ifndef PROJ_MAK_DIR
    ifneq (,$(findstring undefined,$(IMG_CPU)))
	    export PROJ_MAK_DIR = $(IMG_WORKROOT)/build/$(IMG_PROJ)/$(IMG_OS)
    else
	    export PROJ_MAK_DIR = $(IMG_WORKROOT)/build/$(IMG_PROJ)/$(IMG_CPU)
    endif
 endif

-include $(PROJ_MAK_DIR)/help.inc

###############################################
###############################################
##
##	Generic include list
##
ifdef THREAD
  INCLUDES  = -I$(IMG_WORKROOT)/include
endif

###############################################
###############################################
##
##	Generic CFLAGS, LIBS
##
ifdef CFLAGS
CFLAGS += -D__IMG_PRIVATE_APIS__
else
CFLAGS = -D__IMG_PRIVATE_APIS__
endif

LIBS =

export _GMAKE_VERSION=_1.2.2b1


###############################################
###############################################
##
##	CPU target
##
##	meta/win32
##

ifndef IMG_CPU
 $(error ** ERROR: IMG_CPU Not specified )
endif

ifneq (,$(findstring undefined,$(IMG_CPU)))
  ifneq (,$(findstring linux,$(IMG_OS)))
	include $(MAK_DIR)/cpu_linux.inc
  endif
else
	include $(MAK_DIR)/cpu_$(IMG_CPU).inc
endif



###############################################
###############################################
##
##	Release Target
##
##	debug/test/release
##

ifndef RELEASE
export RELEASE=debug
$(warning **WARNING** RELEASE Not specified. default: $(RELEASE) )
endif

ifneq (,$(findstring debug,$(RELEASE)))
RELDEF = 1
endif

ifneq (,$(findstring test,$(RELEASE)))
RELDEF = 1
endif

ifneq (,$(findstring release,$(RELEASE)))
RELDEF = 1
endif

ifndef RELDEF
$(error ** ERROR: Unknown Release '$(RELEASE)' )
endif

include $(MAK_DIR)/rel_$(RELEASE).inc



###############################################
###############################################
##
##	PROJ target
##
##	meson/neutrino_a/neutrino_d etc.
##

ifndef IMG_PROJ
  $(error ** ERROR: IMG_PROJ Not specified )
endif

include $(PROJ_MAK_DIR)/proj_$(IMG_PROJ).inc


###############################################
###############################################
##
##	OS target
##
##	meos/nucleus/win32 etc.
##

ifdef THREAD
  ifndef IMG_OS
    export IMG_OS=meosVx.xx
    $(warning **WARNING** IMG_OS Not specified. default: $(IMG_OS) )
  endif

  ifneq (,$(findstring none,$(IMG_OS)))
    OSDEF = none
  endif

  ifneq (,$(findstring meos,$(IMG_OS)))
    OSDEF = meos
  endif

  ifneq (,$(findstring nucleus,$(IMG_OS)))
    OSDEF = nucleus
  endif

  ifneq (,$(findstring uiplus,$(IMG_OS)))
    OSDEF = uiplus
  endif

  ifneq (,$(findstring win32,$(IMG_OS)))
    OSDEF = win32
  endif

  ifneq (,$(findstring linux,$(IMG_OS)))
    OSDEF = none
  endif

  ifndef OSDEF
    $(error ** ERROR: Unknown OS '$(IMG_OS)' )
  endif

  include $(MAK_DIR)/os_$(OSDEF).inc
endif

###############################################
###############################################
##
##	Hardware target
##
##	emu/sim/fpga etc.
##

ifndef IMG_HW_NOLIB
  ifndef IMG_HW
   $(error ** ERROR: IMG_HW Not specified )
  endif

  include $(PROJ_MAK_DIR)/hw_$(IMG_HW).inc
endif


###############################################
###############################################
##
##	Thread
##
ifneq (,$(findstring thread0,$(THREAD)))
CFLAGS += -D__THREAD0__
endif

ifneq (,$(findstring thread1,$(THREAD)))
CFLAGS += -D__THREAD1__
endif

ifneq (,$(findstring thread2,$(THREAD)))
CFLAGS += -D__THREAD2__
endif

ifneq (,$(findstring thread3,$(THREAD)))
CFLAGS += -D__THREAD3__
endif

ifneq (,$(findstring threadg,$(THREAD)))
CFLAGS += -D__THREADG__
endif

ifneq (,$(findstring threadl,$(THREAD)))
CFLAGS += -D__THREADL__
endif

###############################################
###############################################
##
##	Processor ID
##
ifneq (,$(findstring processor0,$(PROC_ID)))
CFLAGS += -D__PROCESSOR0__
endif

ifneq (,$(findstring processor1,$(PROC_ID)))
CFLAGS += -D__PROCESSOR1__
endif

ifneq (,$(findstring processor2,$(PROC_ID)))
CFLAGS += -D__PROCESSOR2__
endif

ifneq (,$(findstring processor3,$(PROC_ID)))
CFLAGS += -D__PROCESSOR3__
endif

###############################################
###############################################
##
##	LOG_EVENTS
##

ifdef LOG_EVENTS
  CFLAGS += -D_LOG_EVENTS_
endif

###############################################
###############################################
##
##	EXCLUDE ENSIGMA LIBRARIES
##

ifdef EXCLUDE_ENSIGMA_APIS
  CFLAGS += -D_EXCLUDE_ENSIGMA_APIS_
endif

###############################################
###############################################
##
##	TAL and TAL_LIGHT
##

ifdef TAL_LIGHT
  CFLAGS += -D_TAL_LIGHT_
else
  ifdef IMG_TAL
    ifeq ($(IMG_TAL),none)
      CFLAGS += -DIMG_NO_TAL
    else
      CFLAGS += -DIMG_TAL=$(IMG_TAL)
    endif
  else
    CFLAGS += -DIMG_NO_TAL
  endif
endif

###############################################
###############################################
##
##	APP_CFLAGS
##

ifdef APP_CFLAGS
  CFLAGS += $(APP_CFLAGS)
endif

###############################################
###############################################
##
##	APP_CPPFLAGS
##

ifdef APP_CPPFLAGS
  CPPFLAGS += $(APP_CPPFLAGS)
endif

###############################################
###############################################
##
##	APP_LDFLAGS
##

ifdef APP_LDFLAGS
  LDFLAGS += $(APP_LDFLAGS)
endif
