/*!
******************************************************************************
 @file   : nvram_api.c

 @brief

 @Author Imagination Technologies

 @date   10/6/2010

         <b>Copyright 2010 by Imagination Technologies Limited.</b>\n

         All rights reserved.  No part of this software, either material or
         conceptual may be copied or distributed, transmitted, transcribed,
         stored in a retrieval system or translated into any human or computer
         language in any form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to third parties without the express written
         permission of Imagination Technologies Limited, Unit 8, HomePark
         Industrial Estate, King's Langley, Hertfordshire, WD4 8LZ, U.K.

 \n<b>Description:</b>\n
         This file implements the NVRAM API.

 \n<b>Platform:</b>\n
         Saturn

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: nvram_api.c,v $


*****************************************************************************/


/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <MeOS.h>

#include <stdio.h>
#include <string.h>
#include <ioblock_defs.h>
#include <gdma_api.h>
#include <nvram_api.h>

#define NVRAM_DEFAULT_INSTR_WRITE	(0x02)
#define NVRAM_DEFAULT_INSTR_READ	(0x03)
#define NVRAM_DEFAULT_INSTR_WREN	(0x06)
#define NVRAM_DEFAULT_INSTR_RDSR	(0x05)

typedef struct NVRAM_tag_sContext
{
	IMG_BOOL		bInitialised;
	
	SPIM_sBlock	*	psBlock;
	
	NVRAM_sDevice *	psScratchPadDevice;
	IMG_UINT32		ui32ScratchPadPage;
	IMG_UINT8	*	pui8ScratchBuffer;
	IMG_UINT32		ui32ScratchSize;

} NVRAM_sContext;

typedef NVRAM_eResult nvram_WriteFunc(	NVRAM_sDevice	*	psDevice,
										img_uint32			ui32Address,
										img_uint8		*	pui8Buffer,
										img_uint32			ui32NumBytes );
							
static NVRAM_sContext		g_sNVRAMContext = {IMG_FALSE, IMG_FALSE };

/******************************************************************************

    Function:   nvram_FormtInstruction

 ******************************************************************************/
void nvram_FormatInstruction(	img_uint8		*	pui8Buffer,
								img_uint8			ui8Instr,
								img_uint32			ui32Address,
								img_uint8			ui8AddressWidth )
{
	pui8Buffer[0] = ui8Instr;
	switch ( ui8AddressWidth )
	{
		case 4:
		{
			pui8Buffer[1] = (unsigned char)( ( ui32Address >> 24 ) & 0xFF );
			pui8Buffer[2] = (unsigned char)( ( ui32Address >> 16 ) & 0xFF );
			pui8Buffer[3] = (unsigned char)( ( ui32Address >>  8 ) & 0xFF );
			pui8Buffer[4] = (unsigned char)( ( ui32Address >>  0 ) & 0xFF );
			break;
		}
		case 3:
		{
			pui8Buffer[1] = (unsigned char)( ( ui32Address >> 16 ) & 0xFF );
			pui8Buffer[2] = (unsigned char)( ( ui32Address >>  8 ) & 0xFF );
			pui8Buffer[3] = (unsigned char)( ( ui32Address >>  0 ) & 0xFF );
			break;
		}
		case 2:
		{
			pui8Buffer[1] = (unsigned char)( ( ui32Address >> 8 ) & 0xFF );
			pui8Buffer[2] = (unsigned char)( ( ui32Address >> 0 ) & 0xFF );
			break;
		}
		case 1:
		{
			pui8Buffer[1] = (unsigned char)( ( ui32Address >> 0 ) & 0xFF );
			break;
		}
		default:
		{
			IMG_ASSERT(0);
		}
	}	
}

/******************************************************************************

    Function:   NVRAM_Initialise

 ******************************************************************************/
NVRAM_eResult NVRAM_Initialise( SPIM_sBlock	*	psBlock,
							    img_uint8	*	pui8ScratchBuffer,
								img_uint32		ui32ScratchSize )
{
	IMG_ASSERT( psBlock != IMG_NULL );

	g_sNVRAMContext.psBlock 			= psBlock;
	g_sNVRAMContext.pui8ScratchBuffer 	= pui8ScratchBuffer;
	g_sNVRAMContext.ui32ScratchSize 	= ui32ScratchSize;
	g_sNVRAMContext.psScratchPadDevice 	= IMG_NULL;
	g_sNVRAMContext.ui32ScratchPadPage 	= 0;

	g_sNVRAMContext.bInitialised 		= IMG_TRUE;

	return NVRAM_SUCCESS;
}

/******************************************************************************

    Function:   NVRAM_InitDevice

 ******************************************************************************/
NVRAM_eResult NVRAM_InitDevice(	NVRAM_sDevice	*	psDevice )
{
	IMG_ASSERT( psDevice != IMG_NULL );
	
	IMG_ASSERT ( g_sNVRAMContext.bInitialised == IMG_TRUE );

	if ( !psDevice->bOverrideWriteInstr )
	{
		psDevice->ui8WriteInstr = NVRAM_DEFAULT_INSTR_WRITE;
	}

	if ( !psDevice->bOverrideReadInstr )
	{
		psDevice->ui8ReadInstr = NVRAM_DEFAULT_INSTR_READ;
	}

	if ( !psDevice->bOverrideStatusRegBehaviour )
	{
		psDevice->ui8StatusRegInstr = NVRAM_DEFAULT_INSTR_RDSR;
		psDevice->ui8StatusRegReadyBit = 0;
		psDevice->ui8StatusRegReadyBitReadyValue = 0x0;
	}
	
	psDevice->bDeviceInitialised = IMG_TRUE;
	
	return NVRAM_SUCCESS;
}

/******************************************************************************

    Function:   nvram_WaitForWriteReady

 ******************************************************************************/
NVRAM_eResult nvram_WaitForWriteReady(	NVRAM_sDevice	*	psDevice )
{
	img_uint8		aui8Cmd[3];
	SPIM_sBuffer	sWriteBuffer, sReadBuffer;
#if defined __RELEASE_DEBUG__
	static img_uint32 ui32NumWaits = 0;
#endif

	IMG_ASSERT ( g_sNVRAMContext.bInitialised == IMG_TRUE );
	IMG_ASSERT ( psDevice->bDeviceInitialised == IMG_TRUE );

	aui8Cmd[0] = psDevice->ui8StatusRegInstr;

	// Write the status register
	sWriteBuffer.pui8Buffer		= aui8Cmd;
	sWriteBuffer.ui32Size		= 1;
	sWriteBuffer.i32Read		= IMG_FALSE;
	sWriteBuffer.i32Cont		= 1;
	sWriteBuffer.eChipSelect	= psDevice->eChipSelect;
	sWriteBuffer.ui8CmpValue	= 0;
	sWriteBuffer.ui8CmpMask		= 0;
	sWriteBuffer.iCmpEq			= 0;
	sWriteBuffer.i32CmpData		= IMG_NULL;
	sWriteBuffer.i32InterByteDelay = 0;
	// Read the status register
	sReadBuffer.pui8Buffer		= aui8Cmd;
	sReadBuffer.ui32Size		= 1;
	sReadBuffer.i32Read			= IMG_TRUE;
	sReadBuffer.i32Cont			= 0;
	sReadBuffer.eChipSelect		= psDevice->eChipSelect;
	sReadBuffer.ui8CmpValue		= 0;
	sReadBuffer.ui8CmpMask		= 0;
	sReadBuffer.iCmpEq			= 0;
	sReadBuffer.i32CmpData		= IMG_NULL;
	sReadBuffer.i32InterByteDelay = 0;

	while ( 1 )
	{
		if ( SPIMReadWrite( g_sNVRAMContext.psBlock,
							&sWriteBuffer,
							&sReadBuffer,
							IMG_NULL ) != SPIM_OK )
		{
			IMG_ASSERT(0);
			break;
		}

		// Check if the WIP bit is not set
		if ( ( ( aui8Cmd[0] >> psDevice->ui8StatusRegReadyBit ) & 0x01 ) != psDevice->ui8StatusRegReadyBitReadyValue )
		{
			// Reset the byte to the read status register command
				aui8Cmd[0] = psDevice->ui8StatusRegInstr;

#if defined __RELEASE_DEBUG__
			++ui32NumWaits;
#endif
		}
		else
		{
			break;
		}
	}

	return NVRAM_SUCCESS;
}

/******************************************************************************

    Function:   nvram_APIBlockErase

 ******************************************************************************/
NVRAM_eResult nvram_APIBlockErase( NVRAM_sDevice	*	psDevice,
								   img_uint32			ui32Address )
{
	IMG_UINT32	ui32BlockNumber = ui32Address / psDevice->ui32BlockSize;
	
	IMG_ASSERT ( g_sNVRAMContext.bInitialised == IMG_TRUE );
	IMG_ASSERT ( psDevice->bDeviceInitialised == IMG_TRUE );
	IMG_ASSERT( g_sNVRAMContext.pui8ScratchBuffer != IMG_NULL );
	IMG_ASSERT( psDevice->ui32BlockSize <= g_sNVRAMContext.ui32ScratchSize );

	memset( g_sNVRAMContext.pui8ScratchBuffer, 0, g_sNVRAMContext.ui32ScratchSize );
	
	NVRAM_WriteBlock( psDevice, ui32Address, g_sNVRAMContext.pui8ScratchBuffer );
	
	g_sNVRAMContext.psScratchPadDevice = psDevice;
	g_sNVRAMContext.ui32ScratchPadPage = ui32BlockNumber;

	return NVRAM_SUCCESS;
}

/******************************************************************************

    Function:   NVRAM_BlockErase

 ******************************************************************************/
NVRAM_eResult NVRAM_BlockErase(	NVRAM_sDevice	*	psDevice,
								img_uint32			ui32Address )
{
	IMG_ASSERT ( g_sNVRAMContext.bInitialised == IMG_TRUE );
	IMG_ASSERT ( psDevice->bDeviceInitialised == IMG_TRUE );
	
	if ( psDevice->bUseAPIErase )
	{
		return nvram_APIBlockErase( psDevice, ui32Address );
	}
	else
	{
		img_uint8	aui8Cmd[10];

		// Use the NV memory's block erase instruction!
		SPIM_sBuffer		sWriteBuffer;

		sWriteBuffer.pui8Buffer		= aui8Cmd;
		sWriteBuffer.i32Read		= IMG_FALSE;
		sWriteBuffer.i32Cont		= 0;
		sWriteBuffer.eChipSelect	= psDevice->eChipSelect;
		sWriteBuffer.ui8CmpValue	= 0;
		sWriteBuffer.ui8CmpMask		= 0;
		sWriteBuffer.iCmpEq			= 0;
		sWriteBuffer.i32CmpData		= IMG_NULL;
		sWriteBuffer.i32InterByteDelay = 0;

		if ( psDevice->bUseWriteEnable )
		{
			nvram_WaitForWriteReady( psDevice );

			// Setup Write Enable instruction	
			aui8Cmd[0] = NVRAM_DEFAULT_INSTR_WREN;

			sWriteBuffer.ui32Size		= 1;

			if ( SPIMReadWrite( g_sNVRAMContext.psBlock,
								&sWriteBuffer,
								IMG_NULL,
								IMG_NULL ) != SPIM_OK )
			{
				return NVRAM_FAILURE;
			}
		}

		nvram_WaitForWriteReady( psDevice );

		// Setup Page erase instruction		
		if ( !psDevice->ui8BlockAddressBitBase )
		{
			nvram_FormatInstruction( aui8Cmd, psDevice->ui8EraseInstr, ui32Address, psDevice->ui8AddressWidth );
		}
		else
		{
			unsigned long ui32BlockAddress = ((ui32Address / psDevice->ui32BlockSize) << psDevice->ui8BlockAddressBitBase) & ~( ( 1 << psDevice->ui8BlockAddressBitBase ) - 1 );
			unsigned long ui32ByteAddress = ui32Address % psDevice->ui32BlockSize;
			nvram_FormatInstruction(	aui8Cmd, 
										psDevice->ui8EraseInstr, 
										ui32BlockAddress | ui32ByteAddress, 
										psDevice->ui8AddressWidth );
		}

		sWriteBuffer.ui32Size		= 1 + psDevice->ui8AddressWidth;

		if ( SPIMReadWrite( g_sNVRAMContext.psBlock,
							&sWriteBuffer,
							IMG_NULL,
							IMG_NULL ) != SPIM_OK )
		{
			return NVRAM_FAILURE;
		}
	}
	
	return NVRAM_SUCCESS;
}

/******************************************************************************

    Function:   nvram_NormalWriteN

 ******************************************************************************/
NVRAM_eResult nvram_NormalWriteN( NVRAM_sDevice		*	psDevice,
								  img_uint32			ui32Address,
								  img_uint8			*	pui8Buffer,
								  img_uint32			ui32NumBytes )
{
	SPIM_sBuffer	sWriteBuffer[2];
	img_uint8		aui8Cmd[10];

	IMG_ASSERT ( g_sNVRAMContext.bInitialised == IMG_TRUE );
	IMG_ASSERT ( psDevice->bDeviceInitialised == IMG_TRUE );

	// Check that were writing only in the block
	IMG_ASSERT( ui32NumBytes <= psDevice->ui32BlockSize - (ui32Address % psDevice->ui32BlockSize) );

	sWriteBuffer[0].pui8Buffer	= aui8Cmd;
	sWriteBuffer[0].eChipSelect	= psDevice->eChipSelect;
	sWriteBuffer[0].ui8CmpValue	= 0;
	sWriteBuffer[0].ui8CmpMask	= 0;
	sWriteBuffer[0].iCmpEq		= 0;
	sWriteBuffer[0].i32CmpData	= IMG_NULL;
	sWriteBuffer[0].i32InterByteDelay = 0;

	if ( psDevice->bUseWriteEnable )
	{
		nvram_WaitForWriteReady( psDevice );

		// Setup Write Enable instruction	
		aui8Cmd[0] = NVRAM_DEFAULT_INSTR_WREN;
		
		sWriteBuffer[0].ui32Size	= 1;
		sWriteBuffer[0].i32Read		= IMG_FALSE;
		sWriteBuffer[0].i32Cont		= 0;

		if ( SPIMReadWrite( g_sNVRAMContext.psBlock,
							&sWriteBuffer[0],
							IMG_NULL,
							IMG_NULL ) != SPIM_OK )
		{
			return NVRAM_FAILURE;
		}
	}

	nvram_WaitForWriteReady( psDevice );

	if ( !psDevice->ui8BlockAddressBitBase )
	{
		nvram_FormatInstruction( aui8Cmd, 
								(psDevice->bOverrideWriteInstr ) ? psDevice->ui8WriteInstr : NVRAM_DEFAULT_INSTR_WRITE, 
								ui32Address, 
								psDevice->ui8AddressWidth );
	}
	else
	{
		unsigned long ui32BlockAddress = ((ui32Address / psDevice->ui32BlockSize) << psDevice->ui8BlockAddressBitBase) & ~( ( 1 << psDevice->ui8BlockAddressBitBase ) - 1 );
		unsigned long ui32ByteAddress = ui32Address % psDevice->ui32BlockSize;
		nvram_FormatInstruction( aui8Cmd, 
								(psDevice->bOverrideWriteInstr ) ? psDevice->ui8WriteInstr : NVRAM_DEFAULT_INSTR_WRITE, 
								ui32BlockAddress | ui32ByteAddress, 
								psDevice->ui8AddressWidth );
	}

	// Setup Write instruction
	sWriteBuffer[0].ui32Size	= 1 + psDevice->ui8AddressWidth;
	sWriteBuffer[0].i32Read		= IMG_FALSE;
	sWriteBuffer[0].i32Cont		= 1;
	
	// Setup Write data
	sWriteBuffer[1].pui8Buffer	= pui8Buffer;
	sWriteBuffer[1].ui32Size	= ui32NumBytes;
	sWriteBuffer[1].i32Read		= IMG_FALSE;
	sWriteBuffer[1].i32Cont		= 0;
	sWriteBuffer[1].eChipSelect	= psDevice->eChipSelect;
	sWriteBuffer[1].ui8CmpValue	= 0;
	sWriteBuffer[1].ui8CmpMask	= 0;
	sWriteBuffer[1].iCmpEq		= 0;
	sWriteBuffer[1].i32CmpData	= IMG_NULL;
	sWriteBuffer[1].i32InterByteDelay = 0;

	if ( SPIMReadWrite( g_sNVRAMContext.psBlock,
						&sWriteBuffer[0],
						&sWriteBuffer[1],
						IMG_NULL ) != SPIM_OK )
	{
		return NVRAM_FAILURE;
	}

	return NVRAM_SUCCESS;
}

/******************************************************************************

    Function:   nvram_APIWriteN

 ******************************************************************************/
NVRAM_eResult nvram_APIWriteN( NVRAM_sDevice	*	psDevice,
							   img_uint32			ui32Address,
							   img_uint8		*	pui8Buffer,
							   img_uint32			ui32NumBytes )
{
	NVRAM_eResult		eResult;
	
	img_uint32			ui32StartOffset = ui32Address % psDevice->ui32BlockSize;
	IMG_UINT32			ui32BlockNumber = ui32Address / psDevice->ui32BlockSize;

	IMG_ASSERT ( g_sNVRAMContext.bInitialised == IMG_TRUE );
	IMG_ASSERT ( psDevice->bDeviceInitialised == IMG_TRUE );

	// Check that were writing only in the block
	IMG_ASSERT( ui32NumBytes <= psDevice->ui32BlockSize - ui32StartOffset );

	// Check that a block will fit into the application provided scratch buffer
	IMG_ASSERT( g_sNVRAMContext.pui8ScratchBuffer != IMG_NULL );
	IMG_ASSERT( psDevice->ui32BlockSize <= g_sNVRAMContext.ui32ScratchSize );

	// Read the block
	eResult = NVRAM_ReadBlock( psDevice, ui32Address, g_sNVRAMContext.pui8ScratchBuffer );
	if ( eResult != NVRAM_SUCCESS )
	{
		return eResult;
	}
	
	// Modify the data
	IMG_MEMCPY( g_sNVRAMContext.pui8ScratchBuffer + ui32StartOffset, pui8Buffer, ui32NumBytes );
	// Write the block back	
 	eResult = NVRAM_WriteBlock( psDevice, ui32Address, g_sNVRAMContext.pui8ScratchBuffer );
 	
	g_sNVRAMContext.psScratchPadDevice = psDevice;
	g_sNVRAMContext.ui32ScratchPadPage = ui32BlockNumber;
	
	return eResult;
}


/******************************************************************************

    Function:   NVRAM_WriteN

 ******************************************************************************/
NVRAM_eResult NVRAM_WriteN(	NVRAM_sDevice	*	psDevice,
							img_uint32			ui32Address,
							img_uint8		*	pui8Buffer,
							img_uint32			ui32NumBytes )
{
	NVRAM_eResult		eResult = NVRAM_SUCCESS;
	img_uint32			ui32BlockAddress;
	img_uint32			ui32NumBlocks;
	img_uint32			ui32StartOffset;
	img_uint8		*	pui8Source = pui8Buffer;
	img_uint32			ui32BytesToWrite;
	img_uint32			ui32BytesLeft;
	img_uint32			ui32LastBlockRemainder;
	img_uint32			ui32LastBlockBytes;
	img_uint32			ui32SpaceLeft;
	img_uint32			i;
	nvram_WriteFunc	*	pfnWriteFunc;

	IMG_ASSERT ( g_sNVRAMContext.bInitialised == IMG_TRUE );
	IMG_ASSERT ( psDevice->bDeviceInitialised == IMG_TRUE );	

	if ( psDevice->bUseAPIWriteN )
	{
		pfnWriteFunc = &nvram_APIWriteN;
	}
	else
	{
		pfnWriteFunc = &nvram_NormalWriteN;
	}

	ui32StartOffset = ui32Address % psDevice->ui32BlockSize;
	ui32BlockAddress = ui32Address - ui32StartOffset;
	
	// Simple case: the amount of data to write fits into one block
	if ( ui32NumBytes <= psDevice->ui32BlockSize - ui32StartOffset )
	{
		return pfnWriteFunc( psDevice, ui32Address, pui8Buffer, ui32NumBytes );		
	}

	// This is the trickier case where the data to be written spans 2 or more blocks :((((((
	// number of blocks = ( start offset + data size + remainder of last block / block size )
	ui32LastBlockBytes = ( ui32StartOffset + ui32NumBytes ) % psDevice->ui32BlockSize;
	if ( ui32LastBlockBytes == 0 )
	{
		ui32LastBlockRemainder = 0;
	}
	else
	{
		ui32LastBlockRemainder = psDevice->ui32BlockSize - ui32LastBlockBytes;
	}
	ui32NumBlocks = ( ui32StartOffset + ui32NumBytes + ui32LastBlockRemainder) 
					/ 
					psDevice->ui32BlockSize;
	ui32BytesLeft = ui32NumBytes;

	for ( i = 0; i < ui32NumBlocks; ++i )
	{		
		// Calculate how much space is left in the block
		ui32SpaceLeft = psDevice->ui32BlockSize - ui32StartOffset;
		// Calculate the number of bytes to modify in this block
		if ( ui32BytesLeft > ui32SpaceLeft )
		{
			ui32BytesToWrite = ui32SpaceLeft;
		}
		else
		{
			ui32BytesToWrite = ui32BytesLeft;
		}

		eResult = pfnWriteFunc( psDevice, ui32BlockAddress + ( i * psDevice->ui32BlockSize ) + ui32StartOffset, pui8Source, ui32BytesToWrite );
		if ( eResult != NVRAM_SUCCESS )
		{
			return eResult;
		}
		
		pui8Source += ui32BytesToWrite;
		// The rest of the data will be written to the beginning of blocks
		ui32StartOffset = 0;
		ui32BytesLeft -= ui32BytesToWrite;
	}

	return eResult;
}

/******************************************************************************

    Function:   NVRAM_ReadN

 ******************************************************************************/
NVRAM_eResult NVRAM_ReadN(	NVRAM_sDevice	*	psDevice,
							img_uint32			ui32Address,
							img_uint8		*	pui8Buffer,
							img_uint32			ui32NumBytes )
{
	SPIM_sBuffer		sWriteBuffer;
	SPIM_sBuffer		sReadBuffer;
	img_uint8			aui8Cmd[10];
	img_uint32			ui32BytesLeft = ui32NumBytes;
	img_uint32			ui32BytesRead = 0;
	img_uint32			ui32BytesToRead;

	IMG_ASSERT ( g_sNVRAMContext.bInitialised == IMG_TRUE );
	IMG_ASSERT ( psDevice->bDeviceInitialised == IMG_TRUE );

	while ( ui32BytesLeft > 0 )
	{
		nvram_WaitForWriteReady( psDevice );

		if ( !psDevice->ui8BlockAddressBitBase )
		{
			nvram_FormatInstruction( aui8Cmd, 
									 psDevice->ui8ReadInstr,
									 ui32Address + ui32BytesRead,
									 psDevice->ui8AddressWidth );
		}
		else
		{
			unsigned long ui32BlockAddress = (((ui32Address + ui32BytesRead) / psDevice->ui32BlockSize) << psDevice->ui8BlockAddressBitBase) & ~( ( 1 << psDevice->ui8BlockAddressBitBase ) - 1 );
			unsigned long ui32ByteAddress = (ui32Address + ui32BytesRead) % psDevice->ui32BlockSize;			
			nvram_FormatInstruction( aui8Cmd, 
									 psDevice->ui8ReadInstr,
									 ui32BlockAddress | ui32ByteAddress,
									 psDevice->ui8AddressWidth );
		}

		// Setup the read instruction
		sWriteBuffer.pui8Buffer			= aui8Cmd;
		sWriteBuffer.ui32Size			= 1 + psDevice->ui8AddressWidth + psDevice->ui8PaddingBytes;
		sWriteBuffer.i32Read			= IMG_FALSE;
		sWriteBuffer.i32Cont			= 1;
		sWriteBuffer.i32InterByteDelay	= 0;
		sWriteBuffer.eChipSelect		= psDevice->eChipSelect;
		sWriteBuffer.ui8CmpValue		= 0;
		sWriteBuffer.ui8CmpMask			= 0;
		sWriteBuffer.iCmpEq				= 0;
		sWriteBuffer.i32CmpData			= IMG_NULL;

		if ( ui32BytesLeft > SPIM_MAX_TRANSFER_BYTES )
		{
			ui32BytesToRead = SPIM_MAX_TRANSFER_BYTES;
		}
		else
		{
			ui32BytesToRead = ui32BytesLeft;
		}

		// Setup the data read
		sReadBuffer.pui8Buffer			= pui8Buffer + ui32BytesRead;
		sReadBuffer.ui32Size			= ui32BytesToRead;
		sReadBuffer.i32Read				= IMG_TRUE;
		sReadBuffer.i32Cont				= 0;
		sReadBuffer.i32InterByteDelay	= 0;
		sReadBuffer.eChipSelect			= psDevice->eChipSelect;
		sReadBuffer.ui8CmpValue			= 0;
		sReadBuffer.ui8CmpMask			= 0;
		sReadBuffer.iCmpEq				= 0;
		sReadBuffer.i32CmpData			= IMG_NULL;

		// Do it
		if ( SPIMReadWrite( g_sNVRAMContext.psBlock,
							&sWriteBuffer,
							&sReadBuffer,
							IMG_NULL ) != SPIM_OK )
		{
			return NVRAM_FAILURE;
		}

		ui32BytesRead += ui32BytesToRead;
		ui32BytesLeft -= ui32BytesToRead;
	}

	return NVRAM_SUCCESS;
}

/******************************************************************************

    Function:   NVRAM_WriteBlock

 ******************************************************************************/
NVRAM_eResult NVRAM_WriteBlock(	NVRAM_sDevice	*	psDevice,
								img_uint32			ui32Address,
								img_uint8		*	pui8Buffer )							
{
	SPIM_sBuffer		sWriteBuffer[2];
	img_uint8			aui8Cmd[10];

	img_uint32			ui32BlockAddress = ui32Address - (ui32Address % psDevice->ui32BlockSize);

	IMG_ASSERT ( g_sNVRAMContext.bInitialised == IMG_TRUE );
	IMG_ASSERT ( psDevice->bDeviceInitialised == IMG_TRUE );

	sWriteBuffer[0].pui8Buffer	= aui8Cmd;
	sWriteBuffer[0].eChipSelect	= psDevice->eChipSelect;
	sWriteBuffer[0].ui8CmpValue	= 0;
	sWriteBuffer[0].ui8CmpMask	= 0;
	sWriteBuffer[0].iCmpEq		= 0;
	sWriteBuffer[0].i32CmpData	= IMG_NULL;
	sWriteBuffer[0].i32InterByteDelay = 0;

	if ( psDevice->bUseWriteEnable )
	{
		nvram_WaitForWriteReady( psDevice );

		aui8Cmd[0] = NVRAM_DEFAULT_INSTR_WREN;
		// Setup Write Enable instruction
		sWriteBuffer[0].ui32Size	= 1;
		sWriteBuffer[0].i32Read		= IMG_FALSE;
		sWriteBuffer[0].i32Cont		= 0;

		if ( SPIMReadWrite( g_sNVRAMContext.psBlock,
							&sWriteBuffer[0],
							IMG_NULL,
							IMG_NULL ) != SPIM_OK )
		{
			return NVRAM_FAILURE;
		}
	}

	nvram_WaitForWriteReady( psDevice );

	if ( !psDevice->ui8BlockAddressBitBase )
	{
		nvram_FormatInstruction( aui8Cmd, 
								psDevice->ui8WriteInstr, 
								ui32BlockAddress, 
								psDevice->ui8AddressWidth );
	}
	else
	{
		unsigned long ui32BlockAddress = ((ui32Address / psDevice->ui32BlockSize) << psDevice->ui8BlockAddressBitBase) & ~( ( 1 << psDevice->ui8BlockAddressBitBase ) - 1 );
		nvram_FormatInstruction( aui8Cmd, 
								psDevice->ui8WriteInstr, 
								ui32BlockAddress, 
								psDevice->ui8AddressWidth );
	}

	// Setup Write instruction
	sWriteBuffer[0].ui32Size	= 1 + psDevice->ui8AddressWidth;
	sWriteBuffer[0].i32Read		= IMG_FALSE;
	sWriteBuffer[0].i32Cont		= 1;
	
	// Setup Write data
	sWriteBuffer[1].pui8Buffer	= pui8Buffer;
	sWriteBuffer[1].ui32Size	= psDevice->ui32BlockSize;
	sWriteBuffer[1].i32Read		= IMG_FALSE;
	sWriteBuffer[1].i32Cont		= 0;
	sWriteBuffer[1].eChipSelect	= psDevice->eChipSelect;
	sWriteBuffer[1].ui8CmpValue	= 0;
	sWriteBuffer[1].ui8CmpMask	= 0;
	sWriteBuffer[1].iCmpEq		= 0;
	sWriteBuffer[1].i32CmpData	= IMG_NULL;
	sWriteBuffer[1].i32InterByteDelay = 0;

	if ( SPIMReadWrite( g_sNVRAMContext.psBlock,
						&sWriteBuffer[0],
						&sWriteBuffer[1],
						IMG_NULL ) != SPIM_OK )
	{
		return NVRAM_FAILURE;
	}

	return NVRAM_SUCCESS;
}

/******************************************************************************

    Function:   NVRAM_ReadBlock

 ******************************************************************************/
NVRAM_eResult NVRAM_ReadBlock(	NVRAM_sDevice	*	psDevice,
								img_uint32			ui32Address,
								img_uint8		*	pui8Buffer )
{
	img_uint32		ui32BlockAddress = ui32Address - ( ui32Address % psDevice->ui32BlockSize );

	return NVRAM_ReadN( psDevice, ui32BlockAddress, pui8Buffer, psDevice->ui32BlockSize );
}

/******************************************************************************

    Function:   NVRAM_fopen

 ******************************************************************************/
FILE * NVRAM_fopen ( NVRAM_sDevice *	psDevice,
					 NVRAM_sFile *		psFileHandle,
					 IMG_UINT32			ui32NVRAMAddress )
{
	IMG_ASSERT ( psDevice != IMG_NULL );
	IMG_ASSERT ( psFileHandle != IMG_NULL );
	
	IMG_ASSERT ( g_sNVRAMContext.bInitialised == IMG_TRUE );
	IMG_ASSERT ( psDevice->bDeviceInitialised == IMG_TRUE );
	
	if ( ui32NVRAMAddress >= (psDevice->ui32NumBlocks * psDevice->ui32BlockSize) )	/* Specified file is not within the address range of the specified device */
	{
		/* Could not 'open' file */
		return IMG_NULL;	
	}
	
	/* Initialise file handle structure */
	IMG_ASSERT( psFileHandle->bFileOpen == IMG_FALSE );
	IMG_ASSERT( g_sNVRAMContext.pui8ScratchBuffer != IMG_NULL );
	IMG_ASSERT( psDevice->ui32BlockSize <= g_sNVRAMContext.ui32ScratchSize );
	
	psFileHandle->psDevice		= psDevice;
	psFileHandle->ui32ReadIndex	= ui32NVRAMAddress;
	psFileHandle->bFileOpen		= IMG_TRUE;
	
	return ((FILE *) psFileHandle);
}


/******************************************************************************

    Function:   NVRAM_fread

 ******************************************************************************/
 IMG_INT32	NVRAM_fread ( 	IMG_VOID *		pvBuffer,
 							size_t			ui32Size,
 							size_t			ui32Count,
 							FILE *			pFileHandle   )
{
	NVRAM_sFile *	psFile;
	IMG_UINT32		ui32NextPageBoundary = 0;
	IMG_UINT32		ui32EndAddress;	
	IMG_UINT32		ui32TotalBytesToRead;
	IMG_UINT32		ui32LeadingPartialPage;
	IMG_UINT32		ui32TrailingPartialPage;
	IMG_UINT32		ui32TrailingPartialPageStartAddress = 0;
	IMG_UINT32		ui32FullPages;
	IMG_UINT32		ui32FullPageStartAddress = 0;
	IMG_UINT32		ui32BytesRemaining;
	IMG_UINT32		ui32PageNumber;
	IMG_UINT8 *		pui8TargetAddress;
	NVRAM_eResult	eResult;
	
	IMG_ASSERT ( g_sNVRAMContext.bInitialised == IMG_TRUE );
	IMG_ASSERT ( ui32Size != 0 );
	IMG_ASSERT ( ui32Count != 0 );
	IMG_ASSERT ( pvBuffer != IMG_NULL );
	IMG_ASSERT ( pFileHandle != IMG_NULL );
	
	psFile = (NVRAM_sFile *) pFileHandle;
	
	IMG_ASSERT ( psFile->bFileOpen == IMG_TRUE );
	IMG_ASSERT ( psFile->psDevice != IMG_NULL );
	IMG_ASSERT ( psFile->psDevice->bDeviceInitialised == IMG_TRUE );
	
	ui32TotalBytesToRead = (IMG_UINT32) (ui32Size * ui32Count);
	ui32EndAddress = psFile->ui32ReadIndex + ui32TotalBytesToRead;
	if ( ui32EndAddress > (psFile->psDevice->ui32NumBlocks * psFile->psDevice->ui32BlockSize) )
	{
		ui32EndAddress = (psFile->psDevice->ui32NumBlocks * psFile->psDevice->ui32BlockSize);	
		ui32TotalBytesToRead = ui32EndAddress - psFile->ui32ReadIndex;
		
		/* Make sure adjusted 'bytes to read' is still an integer number of units */
		ui32TotalBytesToRead = ui32TotalBytesToRead & ~(ui32Size-1);
	}
	
	if ( (psFile->ui32ReadIndex % psFile->psDevice->ui32BlockSize) != 0 )
	{
		ui32NextPageBoundary = (((psFile->ui32ReadIndex / psFile->psDevice->ui32BlockSize) + 1) * psFile->psDevice->ui32BlockSize);
		ui32LeadingPartialPage = (ui32NextPageBoundary - psFile->ui32ReadIndex);
	}
	else
	{
		/* Read starts on a page boundary - a read of less than a page will be treated as a trailing partial page */
		ui32LeadingPartialPage = 0;
		ui32NextPageBoundary = psFile->ui32ReadIndex;	/* 'Next' page boundary is our current location, on a page boundary */
	}
	
	if ( ui32LeadingPartialPage >= ui32TotalBytesToRead )
	{
		/* Entire read fits within one page */
		ui32LeadingPartialPage = ui32TotalBytesToRead;
		ui32FullPages = 0;
		ui32TrailingPartialPage = 0;
	}
	else
	{
		/* Read bridges at least one page boundary */
		ui32BytesRemaining = ui32TotalBytesToRead - ui32LeadingPartialPage;
		ui32FullPages = ui32BytesRemaining / psFile->psDevice->ui32BlockSize;
		if ( ui32FullPages > 0 )
		{
			ui32FullPageStartAddress = ui32NextPageBoundary;	
		}
		
		ui32TrailingPartialPage = ui32BytesRemaining % psFile->psDevice->ui32BlockSize;
		if ( ui32TrailingPartialPage > 0 )
		{
			ui32TrailingPartialPageStartAddress = (ui32NextPageBoundary + (ui32FullPages * psFile->psDevice->ui32BlockSize));
		}
	}
	
	/* Load partial page at start, if there is one */
	if ( ui32LeadingPartialPage > 0 )
	{	
		ui32PageNumber = (psFile->ui32ReadIndex / psFile->psDevice->ui32BlockSize);
		
		if (( g_sNVRAMContext.psScratchPadDevice != psFile->psDevice ) ||
			( g_sNVRAMContext.ui32ScratchPadPage != ui32PageNumber ))
		{
			/* Load page into scratch pad */
			eResult = NVRAM_ReadBlock ( psFile->psDevice,
										psFile->ui32ReadIndex,
										g_sNVRAMContext.pui8ScratchBuffer );
										
			IMG_ASSERT ( eResult == NVRAM_SUCCESS );
			g_sNVRAMContext.psScratchPadDevice = psFile->psDevice;
			g_sNVRAMContext.ui32ScratchPadPage = ui32PageNumber;
		}
		
		/* Now copy the relevant section into user's buffer */
		IMG_MEMCPY ( pvBuffer, (IMG_UINT8 *) g_sNVRAMContext.pui8ScratchBuffer + (psFile->ui32ReadIndex % psFile->psDevice->ui32BlockSize), ui32LeadingPartialPage );
	}
	
	/* Load any full pages straight into the target buffer */
	if ( ui32FullPages > 0 )
	{
		pui8TargetAddress = (((IMG_UINT8 *) pvBuffer) + ui32LeadingPartialPage);
		eResult = NVRAM_ReadN ( psFile->psDevice,
								ui32FullPageStartAddress,
								pui8TargetAddress,
								(ui32FullPages * psFile->psDevice->ui32BlockSize)  );
		IMG_ASSERT ( eResult == NVRAM_SUCCESS );
	}
	
	/* Load partial trailing page, if there is one */
	if ( ui32TrailingPartialPage > 0 )
	{	
		ui32PageNumber = (ui32TrailingPartialPageStartAddress / psFile->psDevice->ui32BlockSize);
		
		if (( g_sNVRAMContext.psScratchPadDevice != psFile->psDevice ) ||
			( g_sNVRAMContext.ui32ScratchPadPage != ui32PageNumber ))
		{
			/* Load page into scratch pad */
			eResult = NVRAM_ReadBlock ( psFile->psDevice,
										ui32TrailingPartialPageStartAddress,
										g_sNVRAMContext.pui8ScratchBuffer );
										
			IMG_ASSERT ( eResult == NVRAM_SUCCESS );
			g_sNVRAMContext.psScratchPadDevice = psFile->psDevice;
			g_sNVRAMContext.ui32ScratchPadPage = ui32PageNumber;			
		}
		
		/* Now copy the relevant section into user's buffer */
		pui8TargetAddress = ((IMG_UINT8 *) pvBuffer) + ui32LeadingPartialPage + (ui32FullPages * psFile->psDevice->ui32BlockSize);
		IMG_MEMCPY ( (IMG_VOID *) pui8TargetAddress, 
					 (IMG_UINT8 *) g_sNVRAMContext.pui8ScratchBuffer, 
					 ui32TrailingPartialPage );
	}
	
	psFile->ui32ReadIndex += ui32TotalBytesToRead;
	
	return ((IMG_INT32) (ui32TotalBytesToRead/ui32Size));
}

/******************************************************************************

    Function:   NVRAM_fclose

 ******************************************************************************/
IMG_VOID	NVRAM_fclose ( FILE *			pFileHandle )
{
	NVRAM_sFile *	psFile;
	
	IMG_ASSERT ( g_sNVRAMContext.bInitialised == IMG_TRUE );
	IMG_ASSERT ( pFileHandle != IMG_NULL );
	
	psFile = (NVRAM_sFile *) pFileHandle;
	IMG_ASSERT ( psFile->bFileOpen == IMG_TRUE );
	
	psFile->bFileOpen = IMG_FALSE;
}
