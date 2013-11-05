/*!
 *****************************************************************************

 @file      mcp310.h
 @brief     UCC runtime extensions for accessing the MCP in 310 emulation mode

 Copyright (c) Imagination Technologies Limited. 2010

 */
#ifndef _MCP310_H_
#define _MCP310_H_

#include "ucctypes.h"

/**
 * Set up mapping from MCP address space to GRAM in 310 emulation mode
 *
 * @param[in]   mcp         Pointer to MCP object
 * @param[in]  wideBase     GRAM address (based at 0) for WIDE memory region.
 * @param[in]  narrowBase   GRAM address (based at 0) for NARROW memory region.

 *
 * @return  true for success, false for failure.
 *
 */
void
MCP310_mapMemoryToGRAM(MCP_T *mcp, UCCP_GRAM_ADDRESS_T wideBase,
                       UCCP_GRAM_ADDRESS_T narrowBase);
/**
 * Load a program image's code from a family stream onto a MCP
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   streamPtr       Pointer to image family stream
 * @param[in]   imageId         Logical id of image code to load
 * @param[out]  activeId        Pointer to active image Id tracking variable for 310 symbol lookup system.
 *
 * @return  true for success, false for failure.
 */
bool
MCP310_loadImage(MCP_T *mcp, uint8_t *streamPtr, int imageId, int *activeId);
/**
 * Load a program image's code from a family stream onto a MCP
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   streamPtr       Pointer to image family stream
 * @param[in]   imageId         Logical id of image code to load
 * @param[out]  activeId        Pointer to active image Id tracking variable for 310 symbol lookup system.
 *
 * @return  true for success, false for failure.
 */
bool
MCP310_loadImageCode(MCP_T *mcp, uint8_t *streamPtr, int imageId, int *activeId);
/**
 * Load a program image's data from a family stream onto a MCP
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   streamPtr       Pointer to image family stream
 * @param[in]   imageId         Logical id of image data to load
 *
 * @return  true for success, false for failure.
 */
bool
MCP310_loadImageData(MCP_T *mcp, uint8_t *streamPtr, unsigned imageId);
/**
 * Query a family stream for information about the logical images it contains
 *
 * @param[in]   streamPtr       Pointer to image family stream
 * @param[out]  familySize      Length of the stream in bytes
 * @param[out]  maxImageId      Maximum logical image id in the stream
 *
 * @return  true for success, false for failure(bad stream Id).
 */
bool
        MCP310_imageInfo(uint8_t *streamPtr, unsigned *familySize,
                         unsigned *maxImageId);
/*
 * MCP data access functions (1) - copying between host memory and MCP memory
 *
 * These are all array based. There are no equivalent "single word" copy functions
 * since the saving would be slight. With short arrays, the processing is dominated by the
 * address mapping calculations.
 */
/**
 * Copy an array of 8-bit fixed point values to 24-bit MCP memory. Each input byte will be
 * left justified in a 24-bit MCP memory word, with the least significant 16 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_write8dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint8_t *data,
                 unsigned count);
/**
 * Copy an array of 8-bit signed values to 24-bit MCP memory. Each input byte will be
 * sign extended into a 24-bit MCP memory word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_write8int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int8_t *data,
                 unsigned count);
/**
 * Copy an array of 8-bit unsigned integer values to 24-bit MCP memory. Each input byte will be
 * right justified in a 24-bit MCP memory word, with the most significant 16 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_write8uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint8_t *data,
                  unsigned count);
/**
 * Copy an array of 16-bit fixed point values to 24-bit MCP memory. Each input word will be
 * left justified in a 24-bit MCP memory word, with the least significant 8 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_write16dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
                  unsigned count);
/**
 * Copy an array of 16-bit signed values to 24-bit MCP memory. Each input word will be
 * sign extended into a 24-bit MCP memory word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_write16int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int16_t *data,
                  unsigned count);
/**
 * Copy an array of 16-bit unsigned integer values to 24-bit MCP memory. Each input word will be
 * right justified in a 24-bit MCP memory word, with the most significant 16 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_write16uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
                   unsigned count);
/**
 * Copy an array of 16-bit fixed point complex values to 24-bit MCP memory.
 * Bits [0..7] of each input word are copied to bits [4..11] of the MCP word
 * Bits [8..15] of each input word are copied to bits [16..23] of the MCP word
 * Bits [0..3] and [12..15] of the MCP word are zeroed.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_write16cpx(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
                  unsigned count);
/**
 * Copy an array of 32-bit fixed point values to 24-bit MCP memory.
 * Bits [8..31] of each input word are copied to the corresponding MCP word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_write32dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
                  unsigned count);
/**
 * Copy an array of 32-bit signed integer values to 24-bit MCP memory.
 * Bits [0..23] of each input word are copied to the corresponding MCP word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_write32int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int32_t *data,
                  unsigned count);
/**
 * Copy an array of 32-bit unsigned integer values to 24-bit MCP memory.
 * Bits [0..23] of each input word are copied to the corresponding MCP word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_write32uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
                   unsigned count);
/**
 * Copy an array of 32-bit fixed point complex values to 24-bit MCP memory.
 * Bits [4..15] of each input word are copied to bits [0..11] of the MCP word
 * Bits [20..31] of each input word are copied to bits [12..23] of the MCP word
 * Bits [0..3] and [16..19] of the input word are ignored.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_write32cpx(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
                  unsigned count);

/**
 * Copy an array of fixed point values from 24-bit MCP memory to 8-bit host memory.
 * Bits [16..23] of each MCP word are copied to the corresponding output byte.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_read8dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint8_t *data,
                unsigned count);
/**
 * Copy an array of integer values from 24-bit MCP memory to 8-bit host memory.
 * Bits [0..7] of each MCP word are copied to the corresponding output byte.
 * No rounding is performed.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_read8int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int8_t *data,
                unsigned count);
/**
 * Copy an array of integer values from 24-bit MCP memory to 8-bit host memory.
 * Bits [0..7] of each MCP word are copied to the corresponding output byte.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_read8uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint8_t *data,
                 unsigned count);
/**
 * Copy an array of fixed point values from 24-bit MCP memory to 16-bit host memory.
 * Bits [8..23] of each MCP word are copied to the corresponding output word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_read16dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
                 unsigned count);
/**
 * Copy an array of integer values from 24-bit MCP memory to 16-bit host memory.
 * Bits [0..15] of each MCP word are copied to the corresponding output word.
 * No rounding is performed.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_read16int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int16_t *data,
                 unsigned count);
/**
 * Copy an array of integer values from 24-bit MCP memory to 16-bit host memory.
 * Bits [0..15] of each MCP word are copied to the corresponding output word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_read16uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
                  unsigned count);
/**
 * Copy an array of fixed point complex values from MCP memory to 16-bit host memory.
 * Bits [4..11] of each input word are copied to bits [0..7] of the host word
 * Bits [12..23] of each input word are copied to bits [8..15] of the host word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_read16cpx(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
                 unsigned count);
/**
 * Copy an array of fixed point values from 24-bit MCP memory to 32-bit host memory.
 * Each MCP word are copied to bits [8..31] of the corresponding output word.
 * Bits [0..7] of the output word are zeroed.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_read32dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
                 unsigned count);
/**
 * Copy an array of integer values from 24-bit MCP memory to 32-bit host memory.
 * Bits [0..23] of each MCP word are sign-extended to the corresponding output word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_read32int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int32_t *data,
                 unsigned count);
/**
 * Copy an array of integer values from 24-bit MCP memory to 32-bit host memory.
 * Bits [0..23] of each MCP word are zero-extended to the corresponding output word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_read32uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
                  unsigned count);
/**
 * Copy an array of fixed point complex values from MCP memory to 32-bit host memory.
 * Bits [0..11] of each input word are copied to bits [4..15] of the host word
 * Bits [12..23] of each input word are copied to bits [20..31] of the host word
 * Bits [0..3] and [16..19] of the host word are zeroed.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Source address in MCP address space
 * @param[out]  data            Pointer to target data array
 * @param[in]   count           Number of data values to copy
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_read32cpx(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
                 unsigned count);

/**
 * Copy multiple times a single 8-bit fixed point value to an array of consecutive 24-bit MCP memory. Each input byte will be
 * left justified in a 24-bit MCP memory word, with the least significant 16 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Length of the array in the MCP memory
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_fill8dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint8_t *data,
                unsigned count);
/**
 * Copy multiple times a single 8-bit signed value to an array of consecutive 24-bit MCP memory. Each input byte will be
 * sign extended into a 24-bit MCP memory word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @returns                     Length of the array in the MCP memory
 * @param[in]   count           Number of data values to copy.
 */
unsigned
MCP310_fill8int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int8_t *data,
                unsigned count);
/**
 * Copy multiple times a single 8-bit unsigned integer value to an array of consecutive 24-bit MCP memory. Each input byte will be
 * right justified in a 24-bit MCP memory word, with the most significant 16 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Length of the array in the MCP memory
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_fill8uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint8_t *data,
                 unsigned count);
/**
 * Copy multiple times a single 16-bit fixed point value to an array of consecutive 24-bit MCP memory. Each input word will be
 * left justified in a 24-bit MCP memory word, with the least significant 8 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Length of the array in the MCP memory
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_fill16dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
                 unsigned count);
/**
 * Copy multiple times a single 16-bit signed value to an array of consecutive 24-bit MCP memory. Each input word will be
 * sign extended into a 24-bit MCP memory word.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Length of the array in the MCP memory
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_fill16int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int16_t *data,
                 unsigned count);
/**
 * Copy multiple times a single 16-bit unsigned integer value to an array of consecutive 24-bit MCP memory. Each input word will be
 * right justified in a 24-bit MCP memory word, with the most significant 16 bits set to zero
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Length of the array in the MCP memory
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_fill16uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
                  unsigned count);
/**
 * Copy multiple times a single 16-bit fixed point complex value to an array of consecutive 24-bit MCP memory.
 * Bits [0..7] of each input word are copied to bits [4..11] of the MCP word
 * Bits [8..15] of each input word are copied to bits [16..23] of the MCP word
 * Bits [0..3] and [12..15] of the MCP word are zeroed.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Length of the array in the MCP memory
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_fill16cpx(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint16_t *data,
                 unsigned count);
/**
 * Copy multiple times a single 32-bit fixed point value to an array of consecutive 24-bit MCP memory.
 * Bits [8..31] of each input word are copied to the corresponding MCP word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Length of the array in the MCP memory
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_fill32dbl(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
                 unsigned count);
/**
 * Copy multiple times a single 32-bit signed integer value to an array of consecutive 24-bit MCP memory.
 * Bits [0..23] of each input word are copied to the corresponding MCP word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Length of the array in the MCP memory
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_fill32int(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, int32_t *data,
                 unsigned count);
/**
 * Copy multiple times a single 32-bit unsigned integer value to an array of consecutive 24-bit MCP memory.
 * Bits [0..23] of each input word are copied to the corresponding MCP word
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Length of the array in the MCP memory
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_fill32uint(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
                  unsigned count);
/**
 * Copy multiple times a single fixed point complex value to an array of consecutive 24-bit MCP memory.
 * Bits [4..15] of each input word are copied to bits [0..11] of the MCP word
 * Bits [20..31] of each input word are copied to bits [12..23] of the MCP word
 * Bits [0..3] and [16..19] of the input word are ignored.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 * @param[in]   data            Pointer to data
 * @param[in]   count           Length of the array in the MCP memory
 * @returns                     Number of items actually copied.
 */
unsigned
MCP310_fill32cpx(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress, uint32_t *data,
                 unsigned count);

/*
 * MCP310 data access functions (2) - direct access to MCP memory as it appears in GRAM.
 * Note that these functions all fail if the MCP address is in the NARROW data region, since
 * the GRAM data mappings don't work sensibly in these cases.
 */

/**
 * Convert a MCP data address to an address in host memory suitable for accessing the
 * MCP data as a (sign-extended) integer. The integer range is limited to the low 24 bits.
 *
 * The MCP target address must be located in WIDE memory.
 * Direct access to NARROW memory is not useful since the hardware does not provide helpful
 * data formats.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 *
 * @returns     Pointer to host mapped GRAM (or NULL if conversion fails)
 */
MCP_GRAM_INT_T *
MCP310_addrInt2host(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress);
/**
 * Convert a MCP data address to an address in host memory suitable for accessing the
 * MCP data as a fixed point (MCP double) value. The fixed point precision is limited
 * to the high 24 bits.
 *
 * The MCP target address must be located in WIDE memory.
 * Direct access to NARROW memory is not useful since the hardware does not provide helpful
 * data formats.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 *
 * @returns     Pointer to host mapped GRAM (or NULL if conversion fails)
 */
MCP_GRAM_DBL_T *
MCP310_addrDbl2host(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress);
/**
 * Convert a MCP data address to an address in host memory suitable for accessing the
 * MCP data as a fixed point complex value. The precision of each complex component is
 * limited to the high 12 bits.
 *
 * The MCP target address must be located in WIDE memory.
 * Direct access to NARROW memory is not useful since the hardware does not provide helpful
 * data formats.
  *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 *
 * @returns     Pointer to host mapped GRAM (or NULL if conversion fails)
 */
MCP_GRAM_CMPLX_T *
MCP310_addrCpx2host(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress);
/**
 * Convert a MCP data address to an address in host memory suitable for accessing the
 * MCP data as a packed byte sequence.
 *
 * The MCP target address must be located in WIDE memory.
 * Direct access to NARROW memory is not useful since the hardware does not provide helpful
 * data formats.
 *
 * @param[in]   mcp             Pointer to MCP object
 * @param[in]   mcpAddress      Target address in MCP address space
 *
 * @returns     Pointer to host mapped GRAM (or NULL if conversion fails)
 */
MCP_GRAM_PKD_T *
MCP310_addrPkd2host(MCP_T *mcp, MCP_DATA_ADDRESS_T mcpAddress);
#endif /* _MCP310_H_ */
