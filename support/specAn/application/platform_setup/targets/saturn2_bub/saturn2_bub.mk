#
# Saturn support
#

SOC=SATURN2
# For DVB-C tuner:
#IF?=80000000
#RF?=STV6110

# For SiLabs DVB-T tuner:
IF?=24575066
RF?=SI2153


ifeq ($(UNIT_TYPE),LOADER)
IMGS_DIRL+=$(CODELINE_ROOT)/support/specAn/application/platform_setup/targets/saturn2_bub
ifdef BUILD_FOR_BOOT_LOADER
IMGS+=target_saturn2_dummy.img
else
IMGS+=target_saturn2_460-230-460.img
endif
endif
