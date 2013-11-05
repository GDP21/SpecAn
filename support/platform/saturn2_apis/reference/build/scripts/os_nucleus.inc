###############################################
###############################################
##
##	Nucleus OS Target
##
##

###############################################
###############################################
##
##	Generate OS Ver name
##

## Nucleus used by meosal v32-bitHWSTATEXT

## Vxxxxxx is the default version.

ifeq ($(IMG_OS),nucleus_Vxxxxxx)
  OSVER = V050812
else
  ifneq (,$(findstring nucleus_V,$(IMG_OS)))
    OSVER = $(subst nucleus_V,V,$(IMG_OS))
  endif
endif

MEOSVER = v32-bitHWSTATEXT


ifndef OSVER
  $(error ** ERROR: Unknown OS Version '$(IMG_OS)' )
endif


###############################################
###############################################
##
##
##


ifdef THREAD
	INCLUDES += -I$(IMG_WORKROOT)/$(OSDEF)/$(OSVER)/include -I$(IMG_WORKROOT)/MEOSAL/$(MEOSVER)/include
	CFLAGS += -D__META_NUCLEUS_PLUS__
	G_CFLAGS += -D__META_NUCLEUS_PLUS__
	ifndef IMG_OS_NOLIB
		LIBS += $(IMG_WORKROOT)/$(OSDEF)/$(IMG_PROJ)/$(IMG_CPU)/$(THREAD)/lib/$(RELEASE)/nucleus_meta.a
		LIBS += $(IMG_WORKROOT)/meosal/$(IMG_PROJ)/$(IMG_CPU)/$(THREAD)/lib/$(RELEASE)/meosal.a
	endif
endif

###############################################
###############################################
