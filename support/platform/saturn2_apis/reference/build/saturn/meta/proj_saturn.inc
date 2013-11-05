###############################################
###############################################
##
##  SATURN PROJ Target
##
##

ifdef THREAD

  ifneq ($(RELEASE),release)
    export GEN_COMBINED_LIBRARY := $(IMG_WORKROOT)/build/$(IMG_PROJ)/make/lib/libSaturn_$(RELEASE)_$(IMG_CPU)-$(patsubst thread%,%,$(THREAD))gcc.a
  else
    export GEN_COMBINED_LIBRARY := $(IMG_WORKROOT)/build/$(IMG_PROJ)/make/lib/libSaturn_$(IMG_CPU)-$(patsubst thread%,%,$(THREAD))gcc.a
  endif

  CFLAGS += -D__IMG_PROJ_SATURN__

  LDFLAGS += -Usg   ## Remove all unused symbols, even if debug is present

  export GDMA_HW_TYPE := GDMA_SYSBUS

  ## Normal build
  ifneq ($(IMG_HW), saturn_boot)
    INCLUDES += -I$(IMG_WORKROOT)/system/$(IMG_PROJ)/include
    INCLUDES += -I$(IMG_WORKROOT)/apis/$(IMG_PROJ)/include
    INCLUDES += -I$(IMG_WORKROOT)/apis/peripherals/include
    INCLUDES += -I$(IMG_WORKROOT)/imglib/include

    LIBS += $(IMG_WORKROOT)/apis/$(IMG_PROJ)/lib/$(IMG_HW)/$(THREAD)/$(RELEASE)/libSaturnAPI_$(THREAD).a
    LIBS += $(IMG_WORKROOT)/apis/peripherals/$(IMG_PROJ)/lib/$(IMG_HW)/$(THREAD)/$(RELEASE)/libPAPI_$(THREAD).a
    LIBS += $(IMG_WORKROOT)/imglib/$(IMG_PROJ)/lib/$(IMG_HW)/$(THREAD)/$(RELEASE)/libimglib.a

  else
  ## Bootrom build
    INCLUDES += -I$(IMG_WORKROOT)/system/$(IMG_PROJ)/include
    INCLUDES += -I$(IMG_WORKROOT)/apis/$(IMG_PROJ)/include
    INCLUDES += -I$(IMG_WORKROOT)/apis/peripherals/include

    LIBS += $(IMG_WORKROOT)/apis/$(IMG_PROJ)/lib/$(IMG_HW)/$(THREAD)/$(RELEASE)/libSaturnAPI_$(THREAD).a
    LIBS += $(IMG_WORKROOT)/apis/peripherals/$(IMG_PROJ)/lib/$(IMG_HW)/$(THREAD)/$(RELEASE)/libPAPI_$(THREAD).a
  endif    

  CFLAGS += -DGDMA_HW_IMPLEMENTATION=$(GDMA_HW_TYPE)

endif
