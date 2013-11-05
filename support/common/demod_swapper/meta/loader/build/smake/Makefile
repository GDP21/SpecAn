UNIT_TYPE=LOADER
UNIT_NAME=comet_meta_loader
GLOBAL_UCC=UCC310
GLOBAL_TARGET=comet
GLOBAL_PROCESSOR=META
GLOBAL_PROCESSOR_VERSION = 2.1                     #smake uses a default othewise and link flags below (esp -mmetac=2.1) appear ignored by smake and error results
                                                   #METAC 2 1  is also defined at top of memory.img

###----------------- PATHS-----------------------------------------------------

SMAKE_ROOT=../../../../../../../ensigmaSDK/smake


###----------------TOOLS-------------------------------------------------------

include $(SMAKE_ROOT)/smakeinit.mk


###----------------SOURCE-------------------------------------------------------

APPS+=$(UNIT_DIR)/..


###--------------LINK OPTIONS--------------------------------------------------

## gcc in the 2.4.0 toolkits defaults to minim output, however, ld/ldlk default to no minim, therefore to disable minim it is only necessary to add the following to your LOADER makefile:
GLOBAL_CFLAGS += 

## remove -mmetac=2.1  smake ignores this and you need to use the new operator:  GLOBAL_PROCESSOR_VERSION=x.y to switch in the correct version number.
UNIT_LDLKFLAGS = -flayout -mminim --minim-stride=0
#UNIT_LDLKFLAGS = -flayout -mmetac=2.1

## explicitly disable partial linking in LOADER makefile with. src for this=HLT
GLOBAL_LDFLAGS += -Uoff

UNIT_IMGS = comet-memory-256MB.img ldlk-comet-256MB.img comet-PS1-256MB-720-240-360MHz.img

include $(SMAKE_ROOT)/smake.mk
