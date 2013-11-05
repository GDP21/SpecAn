/*!
*****************************************************************************

 @file      uccMem.h
 @brief     Memory address definitions and utilities

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifndef _UCC_MEM_H_
#define _UCC_MEM_H_

/*-------------------------------------------------------------------------*/

#include "uccBlockDefs.h"

/*-------------------------------------------------------------------------*/

/** The size of a host GRAM view */
#define UCC_GRAM_HOST_VIEW_SIZE (MEMGBL_SXT - MEMGBL_DBL)

/**
 * Macro to determine if an address is in the GRAM double view
 *
 * @param[in]  addr  GRAM address
 *
 * @return     Non-zero if the address is in the GRAM double view
 */
#define UCC_IS_DBL(addr) (((uint32_t)(addr) & 0xff000000) == MEMGBL_DBL)

/**
 * Macro to determine if an address is in the GRAM sign-extended view
 *
 * @param[in]  addr  GRAM address
 *
 * @return     Non-zero if the address is in the GRAM sign-extended view
 */
#define UCC_IS_SXT(addr) (((uint32_t)(addr) & 0xff000000) == MEMGBL_SXT)

/**
 * Macro to determine if an address is in the GRAM complex view
 *
 * @param[in]  addr  GRAM address
 *
 * @return     Non-zero if the address is in the GRAM complex view
 */
#define UCC_IS_CPX(addr) (((uint32_t)(addr) & 0xff000000) == MEMGBL_CPX)

/**
 * Macro to determine if an address is in the GRAM packed view.
 *
 * @param[in]  addr  GRAM address
 *
 * @return     Non-zero if the address is in the GRAM packed view
 */
 /*
  * Since objects could be placed in cached GRAM as well as in the more usual
  * "raw" view, this test is a little more complex than the others.
  */
#define UCC_IS_PKD(addr) ((((uint32_t)(addr) & 0xff000000) == MEMGBL_PKD) || \
                         (((uint32_t)(addr) & 0xff000000) == 0x3f000000) || \
                         (((uint32_t)(addr) & 0xff000000) == 0xbf000000) || \
                         (((uint32_t)(addr) & 0xff000000) == 0x37000000))
/**
 * Returns the offset part of a GRAM address.
 *
 * @param[in]  addr  GRAM address
 *
 * @return     The offset part of a GRAM address
 */
#define UCC_GRAM_OFFSET(addr) (((uint32_t)(addr)) & 0xffffff)

/**
 * Convert a GRAM address from packed view to DCP code view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_PKD_TO_DCP_CODE_ADDR(addr) (UCC_GRAM_OFFSET(addr))

/**
 * Convert a GRAM address from packed view to DCP data view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_PKD_TO_DCP_DATA_ADDR(addr) (UCC_GRAM_OFFSET(addr)/3)

/**
 * Convert a GRAM address from double view to DCP data view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_DBL_TO_DCP_DATA_ADDR(addr) (UCC_GRAM_OFFSET(addr)/4)

/**
 * Convert a GRAM address from sign-extended view to DCP data view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_SXT_TO_DCP_DATA_ADDR(addr) (UCC_GRAM_OFFSET(addr)/4)

/**
 * Convert a GRAM address from complex view to DCP data view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_CPX_TO_DCP_DATA_ADDR(addr) (UCC_GRAM_OFFSET(addr)/4)

/**
 * Convert a GRAM address from DCP code to DCP data view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_DCP_CODE_TO_DATA_ADDR(addr) ((addr)/3)

/**
 * Convert a GRAM address from DCP data to DCP code view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_DCP_DATA_TO_CODE_ADDR(addr) (3*(addr))

/**
 * Convert a GRAM word offset to an address in the double view.
 *
 * @param[in]  offset  The GRAM word offset to convert
 *
 * @return     The converted address
 */
#define UCC_GRAM_OFFSET_TO_DBL_ADDR(offset) ((uint32_t *)(MEMGBL_DBL + ((offset)*4)))

/**
 * Convert a GRAM word offset to an address in the sign-extended view.
 *
 * @param[in]  offset  The GRAM word offset to convert
 *
 * @return     The converted address
 */
#define UCC_GRAM_OFFSET_TO_SXT_ADDR(offset) ((uint32_t *)(MEMGBL_SXT + ((offset)*4)))

/**
 * Convert a GRAM word offset to an address in the complex view.
 *
 * @param[in]  offset  The GRAM word offset to convert
 *
 * @return     The converted address
 */
#define UCC_GRAM_OFFSET_TO_CPX_ADDR(offset) ((uint32_t *)(MEMGBL_CPX + ((offset)*4)))

/**
 * Convert a GRAM word offset to an address in the packed view.
 *
 * @param[in]  offset  The GRAM word offset to convert
 *
 * @return     The converted address
 */
#define UCC_GRAM_OFFSET_TO_PKD_ADDR(offset) ((uint32_t *)(MEMGBL_PKD + ((offset)*3)))

/**
 * Convert a GRAM address from packed view to double view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_PKD_TO_DBL_ADDR(addr) ((uint32_t *)(((UCC_GRAM_OFFSET(addr)*4)/3) | MEMGBL_DBL))

/**
 * Convert a GRAM address from packed view to sign-extended view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_PKD_TO_SXT_ADDR(addr) ((uint32_t *)(((UCC_GRAM_OFFSET(addr)*4)/3) | MEMGBL_SXT))

/**
 * Convert a GRAM address from packed view to complex view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_PKD_TO_CPX_ADDR(addr) ((uint32_t *)(((UCC_GRAM_OFFSET(addr)*4)/3) | MEMGBL_CPX))

/**
 * Convert a buffer length from packed view to double view.
 *
 * @param[in]  len  The length to convert
 *
 * @return     The converted length
 */
#define UCC_PKD_TO_DBL_LEN(len) (((len)*4)/3)

/**
 * Convert a GRAM address from sign extended view to packed view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_SXT_TO_PKD_ADDR(addr) ((uint8_t *)(((UCC_GRAM_OFFSET(addr)*3)/4) | MEMGBL_PKD))

/**
 * Convert a GRAM buffer length from sign extended view to packed view.
 *
 * @param[in]  len  The length to convert
 *
 * @return     The converted length
 */
#define UCC_SXT_TO_PKD_LEN(len) (((len)*3)/4)

/**
 * Convert a GRAM address from double view to packed view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_DBL_TO_PKD_ADDR(addr) ((uint8_t *)(((UCC_GRAM_OFFSET(addr)*3)/4) | MEMGBL_PKD))

/**
 * Convert a GRAM buffer length from packed view to double view.
 *
 * @param[in]  len  The length to convert
 *
 * @return     The converted length
 */
#define UCC_DBL_TO_PKD_LEN(len) (((len)*3)/4)

/**
 * Convert a GRAM address from double view to sign-extended view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_DBL_TO_SXT_ADDR(addr) ((uint32_t *)(UCC_GRAM_OFFSET(addr) | MEMGBL_SXT))

/**
 * Convert a GRAM address from sign-extended view to double view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_SXT_TO_DBL_ADDR(addr) ((uint32_t *)(UCC_GRAM_OFFSET(addr) | MEMGBL_DBL))

/**
 * Convert a GRAM address from double view to complex view.
 *
 * @param[in]  addr  The address to convert
 *
 * @return     The converted address
 */
#define UCC_DBL_TO_CPX_ADDR(addr) ((uint32_t *)(UCC_GRAM_OFFSET(addr) | MEMGBL_CPX))

/*-------------------------------------------------------------------------*/

#endif /* _UCC_MEM_H_ */

/*-------------------------------------------------------------------------*/
