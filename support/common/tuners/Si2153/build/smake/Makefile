UNIT_TYPE=MOD
UNIT_NAME=si2153_tuner

include $(SMAKE_ROOT)/smakeinit.mk

C_SRC=si2153_tuner.c si2153_scbm$(PORT_DRIVER_SUFFIX).c

SRCDIR1=$(UNIT_DIR)/source/generic/SampleCode

UNIT_HEADER=si2153.h
UNIT_EXTRAHDRS=si2153_tuner.h

UNIT_ABS_EXTRAHDRS=$(SRCDIR1)/Generic_DTV_ATV_Demod.h     \
                   $(SRCDIR1)/Si2153_Commands.h           \
                   $(SRCDIR1)/Si2153_L0_TV_Chassis.h      \
                   $(SRCDIR1)/Si2153_L1_API_TV_Chassis.h  \
                   $(SRCDIR1)/Si2153_L2_TV_Chassis.h      \
                   $(SRCDIR1)/Si2153_Typedefs.h           \
                   $(SRCDIR1)/Si2153_Properties.h

C_SRC+=Si2153_L1_API_TV_Chassis.c \
		Si2153_L1_Commands.c \
		Si2153_L1_Properties.c \
		Si2153_L2_TV_Chassis.c \
		Generic_DTV_ATV_Demod.c

UNIT_INCLUDES+=$(UNIT_DIR)/source/generic/SampleCode

#GLOBAL_OPTIONS+=ERROR_ON_WARNING

include $(SMAKE_ROOT)/smake.mk
