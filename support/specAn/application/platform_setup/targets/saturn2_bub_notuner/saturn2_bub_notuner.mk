#
# Saturn2 support - special build with generic tuner driver
#

SOC=SATURN2
RF?=GENERIC

ifeq ($(UNIT_TYPE),LOADER)
IMGS_DIRL+=$(TARGET_SUPPORT_ROOT)/targets/saturn2_bub
IMGS+=target_saturn2_460-230-460.img
endif
