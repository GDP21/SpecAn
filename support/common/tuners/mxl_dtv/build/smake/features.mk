UNIT_NAME_SUFFIX:=$(UNIT_NAME_SUFFIX)_$(TARGET)

ifneq (, $(findstring SYS_CLOCK_330MHZ, $(FEATURES)))
UNIT_DEFINES+=SCBCLK_RATE_KHZ=330000
endif

ifneq (, $(findstring SYS_CLOCK_300MHZ, $(FEATURES)))
UNIT_DEFINES+=SCBCLK_RATE_KHZ=300000
endif

ifneq (, $(findstring SYS_CLOCK_280MHZ, $(FEATURES)))
UNIT_DEFINES+=SCBCLK_RATE_KHZ=280000
endif

ifneq (, $(findstring SYS_CLOCK_250MHZ, $(FEATURES)))
UNIT_DEFINES+=SCBCLK_RATE_KHZ=250000
endif

ifneq (, $(findstring SYS_CLOCK_240MHZ, $(FEATURES)))
UNIT_DEFINES+=SCBCLK_RATE_KHZ=240000
endif

ifneq (, $(findstring SYS_CLOCK_230MHZ, $(FEATURES)))
UNIT_DEFINES+=SCBCLK_RATE_KHZ=230000
endif

ifneq (, $(findstring SYS_CLOCK_200MHZ, $(FEATURES)))
UNIT_DEFINES+=SCBCLK_RATE_KHZ=200000
endif

ifneq (, $(findstring SYS_CLOCK_180MHZ, $(FEATURES)))
UNIT_DEFINES+=SCBCLK_RATE_KHZ=180000
endif

ifneq (, $(findstring SYS_CLOCK_160MHZ, $(FEATURES)))
UNIT_DEFINES+=SCBCLK_RATE_KHZ=160000
endif

ifneq (, $(findstring SYS_CLOCK_80MHZ, $(FEATURES)))
UNIT_DEFINES+=SCBCLK_RATE_KHZ=80000
endif

ifneq (, $(findstring SCBM_CLOCK_XTAL1, $(FEATURES)))
UNIT_DEFINES+=SCBCLK_RATE_KHZ=24576
endif

ifneq (, $(findstring GIODRV_PERIPHERALS, $(FEATURES)))
PORT_DRIVER_SUFFIX=_giodrv
UNIT_NAME_SUFFIX:=$(UNIT_NAME_SUFFIX)_giodrv
endif

ifneq (, $(findstring MXL5007T, $(FEATURES)))
SRCDIR2=$(UNIT_DIR)/source/generic/MxL5007T
C_SRC+=MxL5007.c MxL5007_API.c 
UNIT_DEFINES+=MXL5007T
UNIT_NAME_SUFFIX:=$(UNIT_NAME_SUFFIX)_MXL5007T
endif

ifneq (, $(findstring MXL201RF, $(FEATURES)))
SRCDIR2=$(UNIT_DIR)/source/generic/MxL201RF
C_SRC+=MxL201RF.c MxL201RF_API.c
UNIT_DEFINES+=MXL201RF
UNIT_NAME_SUFFIX:=$(UNIT_NAME_SUFFIX)_MXL201RF
endif

############### IF FREQUENCY OPTIONS ###############
# These match with those in MxL5007_Common.h and
# MxL201RF_Common.h. Some are not available for both
# tuners 
####################################################

ifneq (, $(findstring IF_FREQ_4_MHZ, $(FEATURES)))
UNIT_DEFINES+= IF_FREQ=4000000
endif

ifneq (, $(findstring IF_FREQ_4p5_MHZ, $(FEATURES)))
UNIT_DEFINES+= IF_FREQ=4500000
endif

ifneq (, $(findstring IF_FREQ_4p57_MHZ, $(FEATURES)))
UNIT_DEFINES+= IF_FREQ=4570000
endif

ifneq (, $(findstring IF_FREQ_5_MHZ, $(FEATURES)))
UNIT_DEFINES+= IF_FREQ=5000000
endif

ifneq (, $(findstring IF_FREQ_5p38_MHZ, $(FEATURES)))
UNIT_DEFINES+= IF_FREQ=5380000
endif

ifneq (, $(findstring IF_FREQ_6_MHZ, $(FEATURES)))
UNIT_DEFINES+= IF_FREQ=6000000
endif

ifneq (, $(findstring IF_FREQ_6p28_MHZ, $(FEATURES)))
UNIT_DEFINES+= IF_FREQ=6280000
endif

ifneq (, $(findstring IF_FREQ_7p2_MHZ, $(FEATURES))) # Only available on MxL201RF
UNIT_DEFINES+= IF_FREQ=7200000
endif

ifneq (, $(findstring IF_FREQ_9p1915_MHZ, $(FEATURES))) # Only available on MxL5007
UNIT_DEFINES+= IF_FREQ=9191500
endif

ifneq (, $(findstring IF_FREQ_35p25_MHZ, $(FEATURES)))
UNIT_DEFINES+= IF_FREQ=35250000
endif

ifneq (, $(findstring IF_FREQ_36_MHZ, $(FEATURES))) # Only available on MxL201RF
UNIT_DEFINES+= IF_FREQ=36000000
endif

ifneq (, $(findstring IF_FREQ_36p15_MHZ, $(FEATURES)))
UNIT_DEFINES+= IF_FREQ=36150000
endif

ifneq (, $(findstring IF_FREQ_44_MHZ, $(FEATURES)))
UNIT_DEFINES+= IF_FREQ=44000000
endif
