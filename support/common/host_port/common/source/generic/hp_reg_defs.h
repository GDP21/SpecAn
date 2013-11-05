/*!
******************************************************************************
 @file   HP_reg_defs.h

 @brief  Host Port Interface


         <b>Copyright 2010 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

\n\n\n

******************************************************************************/

#if !defined (__HP_REG_DEFS_H__)
#define __HP_REG_DEFS_H__

#if (__cplusplus)
extern "C" {
#endif


/* UCCP Host Port Register definitions */

/* -------------------- Register CR_PERIP_HOST_TO_MTX_CMD -------------------- */

#define CR_PERIP_HOST_TO_MTX_CMD_OFFSET           (0x0030)

/* Field CR_PERIP_HOST_DATA */
#define CR_PERIP_HOST_DATA_OFFSET				  CR_PERIP_HOST_TO_MTX_CMD_OFFSET
#define CR_PERIP_HOST_DATA_SHIFT				  (0)
#define CR_PERIP_HOST_DATA_MASK					  (0x7FFFFFFF)
#define CR_PERIP_HOST_DATA_FLAGS				  (REG_VOL)
#define CR_PERIP_HOST_DATA_LENGTH				  (31)

/* Field CR_PERIP_HOST_INT */
#define CR_PERIP_HOST_INT_OFFSET                  CR_PERIP_HOST_TO_MTX_CMD_OFFSET
#define CR_PERIP_HOST_INT_SHIFT                   (31)
#define CR_PERIP_HOST_INT_MASK        		      (0x80000000)
#define CR_PERIP_HOST_INT_FLAGS      		      (REG_VOL)
#define CR_PERIP_HOST_INT_LENGTH      		      (1)

/* Complete Register Definition */
#define CR_PERIP_HOST_TO_MTX_CMD_REG_OFFSET       CR_PERIP_HOST_TO_MTX_CMD_OFFSET
#define CR_PERIP_HOST_TO_MTX_CMD_REG_SHIFT        (0)
#define CR_PERIP_HOST_TO_MTX_CMD_REG_MASK         (0xFFFFFFFF)
#define CR_PERIP_HOST_TO_MTX_CMD_REG_FLAGS        (REG_VOL)
#define CR_PERIP_HOST_TO_MTX_CMD_REG_LENGTH       (32)

/* -------------------- Register CR_PERIP_MTX_TO_HOST_CMD -------------------- */

#define CR_PERIP_MTX_TO_HOST_CMD_OFFSET           (0x0034)

/* Field CR_PERIP_MTX_DATA */
#define CR_PERIP_MTX_DATA_OFFSET				  CR_PERIP_MTX_TO_HOST_CMD_OFFSET
#define CR_PERIP_MTX_DATA_SHIFT					  (0)
#define CR_PERIP_MTX_DATA_MASK					  (0x7FFFFFFF)
#define CR_PERIP_MTX_DATA_FLAGS					  (REG_VOL)
#define CR_PERIP_MTX_DATA_LENGTH				  (31)

/* Field CR_PERIP_MTX_INT */
#define CR_PERIP_MTX_INT_OFFSET					  CR_PERIP_MTX_TO_HOST_CMD_OFFSET
#define CR_PERIP_MTX_INT_SHIFT					  (31)
#define CR_PERIP_MTX_INT_MASK					  (0x80000000)
#define CR_PERIP_MTX_INT_FLAGS					  (REG_VOL)
#define CR_PERIP_MTX_INT_LENGTH					  (1)

/* Complete Register Definition */
#define CR_PERIP_MTX_TO_HOST_CMD_REG_OFFSET       CR_PERIP_MTX_TO_HOST_CMD_OFFSET
#define CR_PERIP_MTX_TO_HOST_CMD_REG_SHIFT        (0)
#define CR_PERIP_MTX_TO_HOST_CMD_REG_MASK         (0xFFFFFFFF)
#define CR_PERIP_MTX_TO_HOST_CMD_REG_FLAGS        (REG_VOL)
#define CR_PERIP_MTX_TO_HOST_CMD_REG_LENGTH       (32)

/* -------------------- Register CR_PERIP_HOST_TO_MTX_ACK -------------------- */

#define CR_PERIP_HOST_TO_MTX_ACK_OFFSET           (0x0038)

/* Field CR_PERIP_MTX_INT_CLR */
#define CR_PERIP_MTX_INT_CLR_OFFSET				  CR_PERIP_HOST_TO_MTX_ACK_OFFSET
#define CR_PERIP_MTX_INT_CLR_SHIFT				  (31)
#define CR_PERIP_MTX_INT_CLR_MASK				  (0x80000000)
#define CR_PERIP_MTX_INT_CLR_FLAGS				  (REG_VOL)
#define CR_PERIP_MTX_INT_CLR_LENGTH				  (1)

/* Complete Register Definition */
#define CR_PERIP_HOST_TO_MTX_ACK_REG_OFFSET       CR_PERIP_HOST_TO_MTX_ACK_OFFSET
#define CR_PERIP_HOST_TO_MTX_ACK_REG_SHIFT        (0)
#define CR_PERIP_HOST_TO_MTX_ACK_REG_MASK         (0xFFFFFFFF)
#define CR_PERIP_HOST_TO_MTX_ACK_REG_FLAGS        (REG_VOL)
#define CR_PERIP_HOST_TO_MTX_ACK_REG_LENGTH       (32)

/* -------------------- Register CR_PERIP_MTX_TO_HOST_ACK -------------------- */

#define CR_PERIP_MTX_TO_HOST_ACK_OFFSET           (0x003C)

/* Field CR_PERIP_HOST_INT_CLR */
#define CR_PERIP_HOST_INT_CLR_OFFSET			  CR_PERIP_MTX_TO_HOST_ACK_OFFSET
#define CR_PERIP_HOST_INT_CLR_SHIFT				  (31)
#define CR_PERIP_HOST_INT_CLR_MASK				  (0x80000000)
#define CR_PERIP_HOST_INT_CLR_FLAGS				  (REG_VOL)
#define CR_PERIP_HOST_INT_CLR_LENGTH			  (1)

/* Complete Register Definition */
#define CR_PERIP_MTX_TO_HOST_ACK_REG_OFFSET       CR_PERIP_MTX_TO_HOST_ACK_OFFSET
#define CR_PERIP_MTX_TO_HOST_ACK_REG_SHIFT        (0)
#define CR_PERIP_MTX_TO_HOST_ACK_REG_MASK         (0xFFFFFFFF)
#define CR_PERIP_MTX_TO_HOST_ACK_REG_FLAGS        (REG_VOL)
#define CR_PERIP_MTX_TO_HOST_ACK_REG_LENGTH       (32)

/* -------------------- Register CR_PERIP_HOST_INT_ENABLE -------------------- */

#define CR_PERIP_HOST_INT_ENABLE_OFFSET           (0x0040)

/* Field CR_PERIP_HOST_INT_EN */
#define CR_PERIP_HOST_INT_EN_OFFSET				  CR_PERIP_HOST_INT_ENABLE_OFFSET
#define CR_PERIP_HOST_INT_EN_SHIFT				  (31)
#define CR_PERIP_HOST_INT_EN_MASK				  (0x80000000)
#define CR_PERIP_HOST_INT_EN_FLAGS				  (REG_VOL)
#define CR_PERIP_HOST_INT_EN_LENGTH				  (1)

/* Complete Register Definition */
#define CR_PERIP_HOST_INT_ENABLE_REG_OFFSET       CR_PERIP_HOST_INT_ENABLE_OFFSET
#define CR_PERIP_HOST_INT_ENABLE_REG_SHIFT        (0)
#define CR_PERIP_HOST_INT_ENABLE_REG_MASK         (0xFFFFFFFF)
#define CR_PERIP_HOST_INT_ENABLE_REG_FLAGS        (REG_VOL)
#define CR_PERIP_HOST_INT_ENABLE_REG_LENGTH       (32)

/* -------------------- Register CR_PERIP_MTX_INT_ENABLE -------------------- */

#define CR_PERIP_MTX_INT_ENABLE_OFFSET           (0x0044)

/* Field CR_PERIP_MTX_INT_EN */
#define CR_PERIP_MTX_INT_EN_OFFSET				  CR_PERIP_MTX_INT_ENABLE_OFFSET
#define CR_PERIP_MTX_INT_EN_SHIFT				  (31)
#define CR_PERIP_MTX_INT_EN_MASK				  (0x80000000)
#define CR_PERIP_MTX_INT_EN_FLAGS				  (REG_VOL)
#define CR_PERIP_MTX_INT_EN_LENGTH				  (1)

/* Complete Register Definition */
#define CR_PERIP_MTX_INT_ENABLE_REG_OFFSET		  CR_PERIP_MTX_INT_ENABLE_OFFSET
#define CR_PERIP_MTX_INT_ENABLE_REG_SHIFT		  (0)
#define CR_PERIP_MTX_INT_ENABLE_REG_MASK		  (0xFFFFFFFF)
#define CR_PERIP_MTX_INT_ENABLE_REG_FLAGS		  (REG_VOL)
#define CR_PERIP_MTX_INT_ENABLE_REG_LENGTH		  (32)

#if (__cplusplus)
}
#endif

#endif  /* __HP_REG_DEFS_H__ */
