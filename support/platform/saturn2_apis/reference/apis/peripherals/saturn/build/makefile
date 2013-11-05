#__TITLE__(Makefile for lib module)*/

include $(MAK_DIR)/make.inc

BUILDDIR=../lib/$(IMG_HW)/$(RELEASE)/
SUBPATH=../../libraries/

ifneq ($(IMG_HW), saturn_boot)
SUBLIBS = 	scbm	\
			sdio \
			spim \
			spis \
			usb \
			usbd \
			uart \
			ioblock_utils \
			gdma \
			diseqc
else
  SUBLIBS =	spim \
			spis \
			gdma \
			usb \
			usbd \
			ioblock_utils
endif

include $(MAK_DIR)/rules.inc
