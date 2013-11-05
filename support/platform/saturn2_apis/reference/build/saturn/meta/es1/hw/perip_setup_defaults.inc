export IMG_META_FREQ 	:= 480mhz
export IMG_SYS_FREQ   	:= 240mhz
export IMG_DDR_FREQ   	:= 480mhz
export IMG_XTAL_FREQ  	:= 24_576mhz
export IMG_DDR          := 64mbWinbond_ddr2

IMG += $(PROJ_MAK_DIR)/$(IMG_HW)/$(IMG_PLATFORM)/perip_setup_$(IMG_META_FREQ)_meta__$(IMG_SYS_FREQ)_sys__$(IMG_DDR_FREQ)_ddr__$(IMG_XTAL_FREQ)_xtal__$(IMG_DDR).img

