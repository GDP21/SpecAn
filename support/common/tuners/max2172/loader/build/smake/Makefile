# Tuner test loader makefile
#	Copyright (C) Imagination Technologies Ltd.

UNIT_TYPE=LOADER
UNIT_NAME=test_loader

GLOBAL_PROCESSOR=META
GLOBAL_PROCESSOR_VERSION=2.1
GLOBAL_TARGET=SATURN_BUB
BOARD=fpga

ifndef UCC
GLOBAL_UCC=320
GLOBAL_UCCP=320_2
endif

GLOBAL_DEFINES+=USE_UCC_RUNTIME

override MOBILETV_ROOT:=$(shell cd ../../../../../../..;pwd)
export MOBILETV_ROOT
export UCCTOOLKIT_ROOT = ../../../../../../../common/uccToolkit

# We expect the ensigmaSDK directory just below top
SMAKE_ROOT=$(MOBILETV_ROOT)/ensigmaSDK/smake

APPS=$(UNIT_DIR)/../app

###############################################################################

GLOBAL_CFLAGS    += -mminim
GLOBAL_ASFLAGS   +=
GLOBAL_LDFLAGS   += -mminim
GLOBAL_LDLKFLAGS += -mminim --minim-stride=0

UNIT_LDLKFLAGS = -flayout -ffixedmmu
UNIT_IMGS = $(UCCTOOLKIT_ROOT)/uccPlatform/uccps/uccp320_2/uccp320_2.img $(UCCTOOLKIT_ROOT)/uccPlatform/socs/saturn/saturn.img $(UCCTOOLKIT_ROOT)/uccPlatform/targets/saturn_bub/saturn_bub_360-180-360.img layout.img

###############################################################################

include $(SMAKE_ROOT)/smake.mk
