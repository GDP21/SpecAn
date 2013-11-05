/*!
*******************************************************************************
 @file   usbd_dat.c

 @brief  USBD Device Driver data


 @author Imagination Technologies

         <b>Copyright 2007 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

 <b>Platform:</b>\n
         MobileTV

*******************************************************************************/

#if defined __META_MEOS__ || defined __MTX_MEOS__ || defined __META_NUCLEUS_PLUS__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <MeOS.h>

#include "img_defs.h"
#include "ioblock_defs.h"

#include <usb_dfuspec.h>
#include "usbd_api.h"
#include "usb_spec.h"
#include "usb_defs.h"
#include "usb_hal.h"

