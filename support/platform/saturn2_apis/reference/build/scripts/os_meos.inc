###############################################
###############################################
##
##	Meos OS Target 
##
##

###############################################
###############################################
##
##	Check OS Ver
##

ifneq (,$(findstring Vx.xx,$(IMG_OS)))
  OSVER = V3.00
endif

ifneq (,$(findstring V3.00,$(IMG_OS)))
  OSVER = V3.00
endif

ifneq (,$(findstring preinstalled,$(IMG_OS)))
  OSVER = preinstalled
endif

ifndef OSVER
  $(error ** ERROR: Unknown OS Version '$(IMG_OS)' )
endif


ifdef THREAD
	ifneq (,$(findstring V3.00,$(OSVER)))
		export MEOS_INCLUDE_DIR=$(IMG_WORKROOT)/$(OSDEF)/$(OSVER)/include 
		INCLUDES += -I$(IMG_WORKROOT)/$(OSDEF)/$(OSVER)/include 
		ifneq (,$(findstring mtx,$(IMG_CPU)))
			CFLAGS += -D__MTX_MEOS__
		else
			ifneq (,$(findstring meta,$(IMG_CPU)))
        CFLAGS += -D__META_MEOS__
      else
        $(error ** ERROR: CPU not supported '$(IMG_CPU)')
      endif
		endif
		MEOSLIB = $(IMG_WORKROOT)/$(OSDEF)/$(IMG_PROJ)/$(IMG_CPU)/$(THREAD)/lib/$(RELEASE)/meos.a 
		ifeq ($(KRN_EXTRA_PRIORITIES), 1)
			CFLAGS += -DKRN_EXTRA_PRIORITIES
			G_CFLAGS += -DKRN_EXTRA_PRIORITIES
		endif
	endif
	ifneq (,$(findstring preinstalled,$(OSVER)))
		ifneq (,$(findstring mtx,$(IMG_CPU)))
			INCLUDES += -I$(METAG_INST_ROOT)/mtxg-local/include/meos
			CFLAGS += -D__MTX_MEOS__
			ifeq ($(RELEASE), debug)
				MEOSLIB = $(METAG_INST_ROOT)/mtxg-local/lib/libMTXMeOSdbg.a 
			else
				MEOSLIB = $(METAG_INST_ROOT)/mtxg-local/lib/libMTXMeOS.a 
			endif
			ifeq ($(KRN_EXTRA_PRIORITIES), 1)
				CFLAGS += -DKRN_EXTRA_PRIORITIES
				G_CFLAGS += -DKRN_EXTRA_PRIORITIES
			endif
		else
		  ifneq (,$(findstring meta,$(IMG_CPU)))
			  INCLUDES += -I$(METAG_INST_ROOT)/metag-local/include/meos
			  CFLAGS += -D__META_MEOS__
			  ifeq ($(RELEASE), debug)
  				MEOSLIB = $(METAG_INST_ROOT)/metag-local/lib/libMETAMeOSdbg.a 
	  		  else
		  		MEOSLIB = $(METAG_INST_ROOT)/metag-local/lib/libMETAMeOS.a 
			  endif
			  ifeq ($(KRN_EXTRA_PRIORITIES), 1)
				  CFLAGS += -DKRN_EXTRA_PRIORITIES
				  G_CFLAGS += -DKRN_EXTRA_PRIORITIES
			  endif
		  else
		    ifneq (,$(findstring mtp,$(IMG_CPU)))
		        INCLUDES += -I$(METAG_INST_ROOT)/metag-local/include/meos
		        CFLAGS += -D__META_MEOS__
		        ifeq ($(RELEASE), debug)
		          MEOSLIB = $(METAG_INST_ROOT)/metag-local/lib/libMETAMeOSdbg.a
				else
 				  MEOSLIB = $(METAG_INST_ROOT)/metag-local/lib/libMETAMeOS.a
				endif
		        ifeq ($(KRN_EXTRA_PRIORITIES), 1)
		          CFLAGS += -DKRN_EXTRA_PRIORITIES
		          G_CFLAGS += -DKRN_EXTRA_PRIORITIES
		        endif
		    else
              $(error ** ERROR: CPU not supported '$(IMG_CPU)' )
            endif
          endif
		endif
	endif
endif


###############################################################
## if building thread
##   if meos lib does not exist
##      MUST_DO_FIRST = $(MEOSLIB)
##   add $(MEOSLIB) to lib list
##
ifdef THREAD
	ifndef IMG_OS_NOLIB
		ifeq (,$(shell ls $(MEOSLIB) 2>/dev/null))
		##$(warning ** MUST BUILD MEOS FIRST **)
		MUST_DO_FIRST += $(MEOSLIB)
		endif

		CRT2 += $(MEOSLIB)
	endif
endif


