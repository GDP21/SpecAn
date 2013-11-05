###############################################
###############################################
##
##	uiplus OS Target 
##
##

###############################################
###############################################
##
##	Check OS Ver
##

###############################################
###############################################
##
##
##

## its really nucleus
export OSDEF=nucleus

## Add another CFLAG

## and library
ifdef THREAD
	CFLAGS += -DUIPLUS
	ifndef IMG_OS_NOLIB
		LIBS += $(IMG_WORKROOT)/$(OSDEF)/$(IMG_PROJ)/$(IMG_CPU)/$(THREAD)/lib/$(RELEASE)/uiplus.a
	endif
endif

## then include all nucleus stuff
include $(MAK_DIR)/os_nucleus.inc

###############################################

