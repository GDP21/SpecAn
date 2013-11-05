/*!
******************************************************************************
 @file   : nvram_api.h

 @brief

 @Author Imagination Technologies

 @date   10/10/2010

         <b>Copyright 2010 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to third parties without the
         express written permission of Imagination Technologies
         Limited, Unit 8, HomePark Industrial Estate,
         King's Langley, Hertfordshire, WD4 8LZ, U.K.

 \n<b>Description:</b>\n
         This file contains the NVRAM API.

 \n<b>Platform:</b>\n
         Saturn

******************************************************************************/

#if !defined (__NVRAM_API_H__)
#define __NVRAM_API_H__

/*============================================================================
====	I N C L U D E S
=============================================================================*/

#include <img_defs.h>
#include <ioblock_defs.h>
#include <gdma_api.h>
#include <spim_api.h>


/*============================================================================
====	E N U M S
=============================================================================*/

/*!
******************************************************************************
 This type defines the NVRAM function return codes.
******************************************************************************/
typedef enum
{
	NVRAM_SUCCESS,
	NVRAM_FAILURE
} NVRAM_eResult;


/*============================================================================
====	T Y P E D E F S
=============================================================================*/

typedef struct NVRAM_tag_sDevice
{
	IMG_BOOL		bDeviceInitialised;
	
	/* Chip select the device is on */
	SPIM_eDevice	eChipSelect;			
	/* Number of blocks(pages) in the device */
	img_uint32		ui32NumBlocks;			
	/* Block(page) size */
	img_uint32		ui32BlockSize;			
	/* Address width, in bytes */
	img_uint8		ui8AddressWidth;		
	/* For devices which don't support arbitrary byte writes. This causes a
	   Block read/modify/write back function to be used. Eg: the M25PE80 flash
	   must be written using this function. */
	img_bool		bUseAPIWriteN;
	/* Set to true to use the custom write instruction, as specified in ui8WriteInstr */
	img_bool		bOverrideWriteInstr;
	/* Custom write instruction, if different from 0x02 */
	img_uint8		ui8WriteInstr;
	/* Sets the erase instruction if the device supports one. */
	img_uint8		ui8EraseInstr;
	/* Set to true if the device doesn't have an explicit erase operation. The erase function used
	   will write a block of 0's to the device. */
	img_bool		bUseAPIErase;
	/* Override status register read instruction */
	img_bool		bOverrideStatusRegBehaviour;
	/* Status register read opcode */
	img_uint8		ui8StatusRegInstr;
	/* Status register ready bit */
	img_uint8		ui8StatusRegReadyBit;
	/* Value of status register ready bit indicating readiness (1/0) */
	img_uint8		ui8StatusRegReadyBitReadyValue;
	/* Override read instruction */
	img_bool		bOverrideReadInstr;
	/* Read instr opcode */
	img_uint8		ui8ReadInstr;
	/* Number of padding bytes after address */
	img_uint8		ui8PaddingBytes;
	/* Use a write enable phase */
	img_bool		bUseWriteEnable;
	/* Block(page) address bit base - use this if the address bytes are split into a block(page) address and byte address 
	   This is usually necessary for devices with non-power-of-two block(page) sizes */
	img_uint8		ui8BlockAddressBitBase;
	
} NVRAM_sDevice;


typedef struct NVRAM_tag_sFile
{
	IMG_BOOL		bFileOpen;
	
	NVRAM_sDevice *	psDevice;	
	IMG_UINT32		ui32ReadIndex;
	
} NVRAM_sFile;

/*============================================================================
====	F U N C T I O N   P R O T O T Y P E S
=============================================================================*/

/*!
******************************************************************************

 @Function              NVRAM_Initialise

 @Description

 This function initialises the NVRAM Api

 @Input     psBlock				: The SPI Master device to use.

 @Input		pui8ScratchBuffer	: An application specified buffer address for the NVRAM API to use internally.
								  This buffer should be at least as large as the greatest device block size.

 @Input		ui32ScratchSize		: The size of the working buffer.

 @Return    NVRAM_eResult		: Either NVRAM_SUCCESS or NVRAM_FAILURE

******************************************************************************/
extern NVRAM_eResult NVRAM_Initialise( SPIM_sBlock	*	psBlock,
									   img_uint8	*	pui8ScratchBuffer,
									   img_uint32		ui32ScratchSize );


/*!
******************************************************************************

 @Function              NVRAM_InitDevice

 @Description

 This function initialises a NVRAM device

 @Input     psDevice			: The target device to initialise

 @Return    NVRAM_eResult		: Either NVRAM_SUCCESS or NVRAM_FAILURE

******************************************************************************/
extern NVRAM_eResult NVRAM_InitDevice( NVRAM_sDevice	*	psDevice	);

/*!
******************************************************************************

 @Function              NVRAM_BlockErase

 @Description

 This function erases a block in a device at the address specified.

 @Input     psDevice		: The target device to operate on.

 @Input		ui32Address		: The address of the block to erase.

 @Return    NVRAM_eResult	: Either NVRAM_SUCCESS or NVRAM_FAILURE

******************************************************************************/
extern NVRAM_eResult NVRAM_BlockErase(	NVRAM_sDevice	*	psDevice,
										img_uint32			ui32Address );

/*!
******************************************************************************

 @Function              NVRAM_WriteN

 @Description

 This function performs a write operation to a location in device memory.

 @Input     psDevice		: The target device to operate on.

 @Input		ui32Address		: The address to write the data to.

 @Input		pui8Buffer		: The address of an application buffer to write from.

 @Input		ui32NumBytes	: The number of bytes to write.

 @Return    NVRAM_eResult	: Either NVRAM_SUCCESS or NVRAM_FAILURE

******************************************************************************/
extern NVRAM_eResult NVRAM_WriteN(	NVRAM_sDevice	*	psDevice,
									img_uint32			ui32Address,
									img_uint8		*	pui8Buffer,
									img_uint32			ui32NumBytes );

/*!
******************************************************************************

 @Function              NVRAM_ReadN

 @Description

 This function performs a read operation from a location in device memory.

 @Input     psDevice		: The target device to operate on.

 @Input		ui32Address		: The address to read the data from.

 @Input		pui8Buffer		: The address of an application buffer to read into.

 @Input		ui32NumBytes	: The number of bytes to read.

 @Return    NVRAM_eResult	: Either NVRAM_SUCCESS or NVRAM_FAILURE

******************************************************************************/
extern NVRAM_eResult NVRAM_ReadN(	NVRAM_sDevice	*	psDevice,
									img_uint32			ui32Address,
									img_uint8		*	pui8Buffer,
									img_uint32			ui32NumBytes );

/*!
******************************************************************************

 @Function              NVRAM_ReadBlock

 @Description

 This function reads a block from device memory.

 @Input     psDevice		: The target device to operate on.

 @Input		ui32Address		: The block address to read the data from.

 @Input		pui8Buffer		: The address of an application buffer to read into.
							  The block size is specified in the NVRAM_sDevice struct.

 @Return    NVRAM_eResult	: Either NVRAM_SUCCESS or NVRAM_FAILURE

******************************************************************************/
extern NVRAM_eResult NVRAM_ReadBlock( NVRAM_sDevice	*	psDevice,
									  img_uint32		ui32Address,
									  img_uint8		*	pui8Buffer );

/*!
******************************************************************************

 @Function              NVRAM_WriteN

 @Description

 This function writes to a block of device memory.

 @Input     psDevice		: The target device to operate on.

 @Input		ui32Address		: The block address to write the data to.

 @Input		pui8Buffer		: The address of an application buffer to write from.
							  The block size is specified in the NVRAM_sDevice struct.

 @Return    NVRAM_eResult	: Either NVRAM_SUCCESS or NVRAM_FAILURE

******************************************************************************/
extern NVRAM_eResult NVRAM_WriteBlock(	NVRAM_sDevice	*	psDevice,
										img_uint32			ui32Address,
										img_uint8		*	pui8Buffer );

/*!
******************************************************************************

 @Function              NVRAM_fopen

 @Description

 Allows sequential reads from a starting point in NVRAM, as if reading from
 a file. Using this method will prevent unnecessary NVRAM accesses when reading
 multiple small amounts of data - a copy of the last page read is kept locally
 to prevent multiple NVRAM reads where multiple read accesses are being made
 from the same NVRAM page.

 @Input     psDevice			: The target device to operate on.

 @Input		psFileHandle		: The address of a user owned 'NVRAM_sFile' structure
 								  which the API can use to keep track of this 'file read'. 							

 @Input		ui32NVRAMAddress	: The NVRAM address at which the 'file' starts.
							  		

 @Return    FILE *				: The function returns a file pointer which must
 								  be provided in subsequent references to this 'file read'.

******************************************************************************/
extern FILE * NVRAM_fopen (	NVRAM_sDevice 	*	psDevice,
							NVRAM_sFile 	*	psFileHandle,
					 		IMG_UINT32			ui32NVRAMAddress );
					 		
/*!
******************************************************************************

 @Function              NVRAM_fread

 @Description

 This function reads data from NVRAM into a provided buffer.

 @Input     pvBuffer		: The target buffer.

 @Input		ui32Size		: The size of the unit to read.
 
 @Input		ui32Count		: The number of units to read.

 @Input		pFileHandle		: A file handle returned when 'NVRAM_fopen' was called.

 @Return    IMG_INT32		: The number of bytes read.

******************************************************************************/					 		
extern IMG_INT32 NVRAM_fread ( IMG_VOID *		pvBuffer,
 							   size_t			ui32Size,
 							   size_t			ui32Count,
 							   FILE *			pFileHandle );					 
 						
/*!
******************************************************************************

 @Function              NVRAM_fclose

 @Description

 This function closes a file previously opened with 'NVRAM_fopen'.

 @Input     pFileHandle		: A file handle returned when 'NVRAM_fopen' was called.

 @Return    None.

******************************************************************************/ 								
extern IMG_VOID	NVRAM_fclose ( FILE *			pFileHandle );

#endif /* __NVRAM_API_H__ */
