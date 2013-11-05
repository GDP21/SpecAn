export IMG_META_FREQ 	:= 400mhz
export IMG_SYS_FREQ   	:= 200mhz
export IMG_DDR_FREQ   	:= no
export IMG_XTAL_FREQ  	:= 24_576mhz
export IMG_DDR          := no_ddr2

IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_setup_$(IMG_META_FREQ)_meta__$(IMG_SYS_FREQ)_sys__$(IMG_DDR_FREQ)_ddr__$(IMG_XTAL_FREQ)_xtal__$(IMG_DDR).img

