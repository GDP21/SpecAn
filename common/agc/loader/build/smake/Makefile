# Documentation loader makefile
#	Copyright (C) Imagination Technologies Ltd.

UNIT_TYPE=LOADER
UNIT_NAME=agc_test_loader

GLOBAL_PROCESSOR=MTX
GLOBAL_TARGET=kuroi2
BOARD=fpga

ifndef UCCP
GLOBAL_UCCP=320_2
endif

override MOBILETV_ROOT:=$(shell cd ../../../../..;pwd)
export MOBILETV_ROOT

# We expect the ensigmaSDK directory just below top
SMAKE_ROOT=$(MOBILETV_ROOT)/ensigmaSDK/smake

APPS=$(UNIT_DIR)/../app

include $(SMAKE_ROOT)/smake.mk
