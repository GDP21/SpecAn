#__TITLE__(Makefile for sublib module)*/

#################################################################
#
#
################################################################


include $(MAK_DIR)/make.inc

INCLUDES += -I$(IMG_WORKROOT)/apis/peripherals/libraries/usb/code

SRCDIR = ../code

SRC =					\
	dwc_otg_pcd.c		\
	dwc_otg_pcd_intr.c	\
	usbd_api.c			\
	usbd_drv.c			\
	usbd_func.c			
	  
ifneq ($(NO_DFU_SUPPORT), true)
SRC += dfu.c
endif
	
include $(MAK_DIR)/rules.inc
