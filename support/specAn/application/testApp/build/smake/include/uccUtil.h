/*!
*****************************************************************************

 @file      uccUtil.h
 @brief     Miscellaneous utilities

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifndef _UCC_UTIL_H_
#define _UCC_UTIL_H_

/*-------------------------------------------------------------------------*/

/*
 * Bit manipulation macros.
 */
 
/**
 * Clear bits.
 *
 * @param[in]  currentVal  Current value
 * @param[in]  mask        Mask of bits to clear
 *
 * @return Modified value
 */
#define UCC_CLR_BITS(currentVal, mask) ((currentVal) & (uint32_t)(~(mask)))

/**
 * Set bits.
 *
 * @param[in]  currentVal  Current value
 * @param[in]  mask        Mask of bits to set
 *
 * @return Modified value
 */
#define UCC_SET_BITS(currentVal, mask) ((currentVal) | (mask))

/**
 * Put bits using:
 * - No range checking
 * - Explictly specified mask and shift values
 *
 * @param[in]  currentVal  Current value
 * @param[in]  fieldVal    Field value to put
 * @param[in]  mask        Mask value
 * @param[in]  shift       Shift value
 *
 * @return Modified value 
 */
#define UCC_PUT_BITS_NO_CHECK_EX(currentVal, fieldVal, mask, shift) (((currentVal) & (~(mask))) | (((fieldVal) << (shift)) & (mask)))

/**
 * Put bits using:
 * - No range checking
 * - Field specified with prefix only
 *
 * @param[in]  currentVal  Current value
 * @param[in]  fieldVal    The field value to put
 * @param[in]  fieldName   Field name (prefix of MASK/SHIFT defines)
 *
 * @return Modified value 
 */
#define UCC_PUT_BITS_NO_CHECK(currentVal, fieldVal, fieldName) UCC_PUT_BITS_NO_CHECK_EX(currentVal, fieldVal, fieldName##_MASK, fieldName##_SHIFT)

/* @cond DONT_DOXYGEN */

/* UCC_PUT_BITS_EX_IMP is the macro that actually implements this
 * functionality. If range checking is enabled, then map this onto the
 * UCC_putBitsEx function. Otherwise map onto the UCC_PUT_BITS_NO_CHECK_EX
 * macro.
 */
#ifdef WITH_FIELD_RANGE_CHECK
uint32_t UCC_putBitsEx(uint32_t currentVal,
                       uint32_t fieldVal,
                       uint32_t mask,
                       uint32_t shift,
                       int      isSigned);
#define UCC_PUT_BITS_EX_IMP(currentVal, fieldVal, mask, shift, isSigned) UCC_putBitsEx(currentVal, (uint32_t)fieldVal, mask, shift, isSigned)
#else
#define UCC_PUT_BITS_EX_IMP(currentVal, fieldVal, mask, shift, isSIgned) UCC_PUT_BITS_NO_CHECK_EX(currentVal, fieldVal, mask, shift)
#endif

/* @endcond */

/**
 * Put bits using:
 * - Range checking (when enabled)
 * - Explictly specified mask and shift values
 *
 * @param[in]  currentVal  Current value
 * @param[in]  fieldVal    The field value to put
 * @param[in]  mask        Mask value
 * @param[in]  shift       Shift value
 * @param[in]  isSigned    Is the field value signed?
 *
 * @return Modified value 
 */
#define UCC_PUT_BITS_EX(currentVal, fieldVal, mask, shift, isSigned) UCC_PUT_BITS_EX_IMP(currentVal, fieldVal, mask, shift, isSigned)

/**
 * Put bits using:
 * - Range checking (when enabled)
 * - Field specified with prefix only
 *
 * @param[in]  currentVal  Current value
 * @param[in]  fieldVal    The field value to put
 * @param[in]  fieldName   Field name (prefix of MASK/SHIFT/SIGNED defines)
 *
 * @return Modified value 
 */
#define UCC_PUT_BITS(currentVal, fieldVal, fieldName) UCC_PUT_BITS_EX_IMP(currentVal, fieldVal, fieldName##_MASK, fieldName##_SHIFT, fieldName##_SIGNED)

/**
 * Get bits.
 *
 * @param[in]  currentVal  Current value
 * @param[in]  fieldName   Field name (prefix of MASK/SHIFT defines)
 *
 * @return Updated value
 */
#define UCC_GET_BITS(currentVal, fieldName) (((currentVal) & (fieldName##_MASK)) >> (fieldName##_SHIFT))

/**
 * Convert a length in bits to a bit mask.
 *
 * @param[in]  length  The length of the mask in bits
 *
 * @return A bit mask of the specified length
 */
#define UCC_LENGTH_TO_MASK(length) ((1UL << length) - 1)
 
/**
 * Convert a shift count and a length in bits to a bit mask.
 *
 * @param[in]  shift   The left shift count of the mask in bits
 * @param[in]  length  The length of the mask in bits
 *
 * @return A bit mask of the specified length, shifted left
 */
#define UCC_LENGTH_TO_SHIFTED_MASK(shift, length) (UCC_LENGTH_TO_MASK(length) << shift)
 
/**
 * Convert the index of an item in a regularly spaced sequence to the offset
 * of the item.
 *
 * @param[in]  offset0  The offset of the first item in the sequence
 * @param[in]  offset1  The offset of the second item in the sequence
 * @param[in]  index    The index of the sought item
 *
 * @return The offset of the sought item 
 */
#define UCC_INDEX_TO_OFFSET(offset0, offset1, index) ((offset0) + (((offset1) - (offset0)) * (index)))
 
/*-------------------------------------------------------------------------*/

/*
 * Endian neutral get/put macros.
 */
 
/**
 * Get an 8-bit word.
 *
 * @param[in]  p  Pointer to byte stream
 */
#define UCC_GET8(p) ((p)[0])

/**
 * Get a 16-bit word.
 *
 * @param[in]  p  Pointer to byte stream
 */
#define UCC_GET16(p) ((p)[0] + ((p)[1] << 8))

/**
 * Get a 24-bit word.
 *
 * @param[in]  p  Pointer to byte stream
 */
#define UCC_GET24(p) ((p)[0] + ((p)[1] << 8) + ((p)[2] << 16))

/**
 * Get a 32-bit word.
 *
 * @param[in]  p  Pointer to byte stream
 */
#define UCC_GET32(p) ((p)[0] + ((p)[1] << 8) + ((p)[2] << 16) + ((p)[3] << 24))

/**
 * Put an 8-bit word.
 *
 * @param[in]  p  Pointer to byte stream
 * @param[in]  x  8-bit value
 */
#define UCC_PUT8(p, x) {uint8_t *lp = (p); *lp++ = (uint8_t)(x);}

/**
 * Put a 16-bit word.
 *
 * @param[in]  p  Pointer to byte stream
 * @param[in]  x  16-bit value
 */
#define UCC_PUT16(p, x) {uint8_t *lp = (p); *lp++ = (uint8_t)((x) & 0xff); *lp++ = (uint8_t)(((x) >> 8) & 0xff);}

/**
 * Put a 24-bit word.
 *
 * @param[in]  p  Pointer to byte stream
 * @param[in]  x  24-bit value
 */
#define UCC_PUT24(p, x) {uint8_t *lp = (p); *lp++ = (uint8_t)((x) & 0xff); *lp++ = (uint8_t)(((x) >> 8) & 0xff); *lp++ = (uint8_t)(((x) >> 16) & 0xff);}

/**
 * Put a 32-bit word.
 *
 * @param[in]  p  Pointer to byte stream
 * @param[in]  x  32-bit value
 */
#define UCC_PUT32(p, x) {uint8_t *lp = (p); *lp++ = (uint8_t)((x) & 0xff); *lp++ = (uint8_t)(((x) >> 8) & 0xff); *lp++ = (uint8_t)(((x) >> 16) & 0xff); *lp++ = (uint8_t)(((x) >> 24) & 0xff);}

/*-------------------------------------------------------------------------*/

/**
 * Align a value.
 *
 * @param[in]  val    Value to align
 * @param[in]  align  Required alignment in bytes
 */
#define UCC_ALIGN(val, align) ((align)*(((val) + (align) - 1)/(align)))

/*-------------------------------------------------------------------------*/

#endif /* _UCC_UTIL_H_ */

/*-------------------------------------------------------------------------*/
