#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <MeOS.h>
#include <img_defs.h>

__attribute__ ((__section__ (".gram_buffer"))) IMG_UINT8 refWriteBuffer1[5120];
