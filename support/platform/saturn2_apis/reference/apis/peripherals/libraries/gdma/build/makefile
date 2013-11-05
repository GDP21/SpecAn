#__TITLE__(Makefile for sublib module)*/

#################################################################
#
#
################################################################


include $(MAK_DIR)/make.inc

SRCDIR = ../code/

SRC =					\
	gdma_api.c			\
	gdma_drv.c
	
ifeq ($(GDMA_HW_TYPE), GDMA_SYSBUS)
$(info *** Building SYSBUS implementation of GDMA driver ***)
SRC += gdma_hal_sysbus.c
else
ifeq ($(GDMA_HW_TYPE), GDMA_MDC)
$(info *** Building MDC implementation of GDMA driver ***)
SRC += gdma_hal_mdc.c
else
$(info *** WARNING: NO MDC HARDWARE IMPLEMENTATION SPECIFIED ***)
endif
endif	
	

include $(MAK_DIR)/rules.inc
