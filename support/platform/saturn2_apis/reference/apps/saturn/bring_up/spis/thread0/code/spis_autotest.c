/*!
******************************************************************************
 @file   : spis_autotest.c

 @brief

 @Author Imagination Technologies

 @date   10/07/2007

         <b>Copyright 2007 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to third parties without the
         express written permission of Imagination Technologies
         Limited, Unit 8, HomePark Industrial Estate,
         King's Langley, Hertfordshire, WD4 8LZ, U.K.

 <b>Description:</b>\n
         SPI Slave Test Application.

 <b>Platform:</b>\n
	     Platform Independent

 @Version
	     1.0

******************************************************************************/


/*============================================================================
====	I N C L U D E S
=============================================================================*/

#define METAG_ALL_VALUES
#define METAC_ALL_VALUES

#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <time.h>
#include <stdlib.h>

#include <MeOS.h>

#include <img_defs.h>

#include <ioblock_defs.h>
#include <gdma_api.h>
#include <sys_config.h>

#include "spis_api.h"

#include "spis_autotest.h"

#define SPIS_LIBRARY_RAND_AVAILABLE
#if defined SPIS_LIBRARY_RAND_AVAILABLE
#else
  #define SPIS_USE_TIMER_FOR_RAND		((IMG_UINT8)(*(unsigned long *)0x04800010))
#endif

/*============================================================================
====	D E F I N E S
=============================================================================*/

#define X25_INIT				0xFFFFU						// X25 CRC bits'n'pieces
#define X25_RES					0x1D0FU

#define MAX_SIZE				5120

/*============================================================================
====	E N U M S
=============================================================================*/

typedef enum
{
	OPER_READ,
	OPER_WRITE,
	OPER_READWRITE
} TEST_eOperation;

/*============================================================================
====	T Y P E D E F S
=============================================================================*/

typedef struct
{
	SPIS_PORT_T				sSPISlave;
	SPIS_IO_BLOCK_T			sIOBlock;

	SPIS_PARAM_T			sTestConfig;
} TEST_sContext;

typedef struct
{
	TEST_eOperation		eOperation;
	unsigned long		ui32NumJobs;
	unsigned long		ui32Length;
	unsigned long		ui32Mode;
	unsigned long		ui32Bitrate;
	unsigned long		ui32CSActive;
	unsigned long		ui32DeviceDMAChannel;
} TEST_sDescription;

typedef struct
{
	unsigned char		ui8Operation;
	unsigned char		ui8Mode;
	unsigned short		ui16Bitrate;
	unsigned short		ui16Length;
	unsigned char		ui8CSActive;
	unsigned char		ui8DMAChannel;
	unsigned short		ui16CRC;
} __attribute__ ((packed)) TEST_sTestDescPacket;

typedef struct
{
	unsigned char	ui8Data[2];
	unsigned short	ui16CRC;
} __attribute__ ((packed)) TEST_sAckPacket;



/*============================================================================
====	D A T A
=============================================================================*/

extern IMG_UINT8			g_aui8ReadBuffer[ MAX_SIZE ];
extern IMG_UINT8			g_aui8RefWriteBuffer[ MAX_SIZE ];
__attribute__ ((__section__ (".bulkbuffers"))) IMG_UINT8 g_aui8Scratch[40];

extern img_uint32			g_ui32BlockNum;

static TEST_sContext		g_sContext;

static SPIS_PARAM_T	g_sTestProto =
{
	SPI_MODE_0,		// Mode 0
	0,				// CSLevel 0
	0,				// DMA 0
	0,				// Standalone
	SPI_SYNC_MODE_LEGACY
};

// CRC Table
static const unsigned short X25Tab[16] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF
};

/*!
******************************************************************************

 @Function				CRCX25

******************************************************************************/
unsigned short CRCX25(	unsigned char	*	pui8Data,
						unsigned long		ui32Size	)
{
	unsigned short ui16CRCReg = X25_INIT;
	unsigned short ui16Temp;
	unsigned short ui8Data = (unsigned char)0;

	/* Use 4 bits out of the data/polynomial at a time */
	do
	{
		/* Get data byte */
		ui8Data = (unsigned short)*pui8Data++;

		/* Use 4 bits out of the data/polynomial at a time */
		ui16Temp = (unsigned short)(ui16CRCReg >> 12u);
		ui16Temp ^= (unsigned short)(ui8Data >> 4u);				// xor data (MS 4 bits) with the MS 4 bits
		ui16Temp &= 0xf;										// of the CRC reg to be used as index in array
		ui16CRCReg = (unsigned short)( ( ui16CRCReg << 4u ) ^ X25Tab[ ui16Temp ] );

		/* Now do second half of byte */
		ui16Temp = (unsigned short)(ui16CRCReg >> 12u);
		ui16Temp ^= ui8Data;									// xor data with the 4 MS bits of the CRC reg
		ui16Temp &= 0xf;										// to be used as index in array
		ui16CRCReg = (unsigned short)( ( ui16CRCReg << 4u ) ^ X25Tab[ ui16Temp ] );

	} while ( (--ui32Size) != 0 );

	return ( (unsigned short)( ui16CRCReg ^ X25_RES ) );
}

/*!
******************************************************************************

 @Function				TEST_ConfigureForProtocol

******************************************************************************/
int TEST_ConfigureForProtocol()
{
	g_sContext.sSPISlave.bInitialised = IMG_FALSE;
	if ( SPISInit( &g_sContext.sSPISlave, (SPIS_PARAM_T *)&g_sTestProto, &g_sContext.sIOBlock, 1 ) != SPIS_OK )
	{
		IMG_ASSERT(0);
		return 0;
	}

	return 1;
}

/*!
******************************************************************************

 @Function				TEST_ConfigureForTest

******************************************************************************/
int TEST_ConfigureForTest(	const TEST_sDescription	*	psTest	)
{
	g_sContext.sSPISlave.bInitialised 		= IMG_FALSE;
	g_sContext.sTestConfig.csLevel			= psTest->ui32CSActive;
	g_sContext.sTestConfig.dmaChannel		= psTest->ui32DeviceDMAChannel;
	g_sContext.sTestConfig.spiMode			= (SPIS_MODE_T)psTest->ui32Mode;
	g_sContext.sTestConfig.ui32BlockIndex   = g_ui32BlockNum;
	g_sContext.sTestConfig.spiSyncMode		= SPI_SYNC_MODE_LEGACY;

	if ( SPISInit( &g_sContext.sSPISlave, &g_sContext.sTestConfig, &g_sContext.sIOBlock, 1 ) != SPIS_OK )
	{
		IMG_ASSERT(0);
		return 0;
	}

	return 1;
}

/*!
******************************************************************************

 @Function				TEST_GenerateData

******************************************************************************/
void TEST_GenerateData( const TEST_sDescription	*	psTest,	unsigned short	*	pui16CRC )
{
	unsigned long i;
	// Fill the buffer with random data
	for ( i = 0; i < psTest->ui32Length; ++i )
	{
#if defined SPIS_LIBRARY_RAND_AVAILABLE
		g_aui8RefWriteBuffer[i] = rand();
#else
		g_aui8RefWriteBuffer[i] = SPIS_USE_TIMER_FOR_RAND;
#endif
	}

	// Calculate a CRC for the data
	IMG_ASSERT( pui16CRC );

	*pui16CRC = CRCX25( g_aui8RefWriteBuffer, psTest->ui32Length );
}

/*!
******************************************************************************

 @Function				TEST_SendAck

******************************************************************************/
int TEST_SendAck(	const unsigned short	*	pui16CRC )
{
	TEST_sAckPacket		*	psAck = (TEST_sAckPacket *)g_aui8Scratch;

#if defined SPIS_LIBRARY_RAND_AVAILABLE
	psAck->ui8Data[0] = rand();
#else
	psAck->ui8Data[0] = SPIS_USE_TIMER_FOR_RAND;
#endif

	psAck->ui8Data[1] = ~psAck->ui8Data[0];

	if ( pui16CRC )
	{
		psAck->ui16CRC = *pui16CRC;
	}

	if ( SPISWrite( &g_sContext.sSPISlave, g_aui8Scratch, sizeof(TEST_sAckPacket), NULL, SPIS_INF_TIMEOUT ) != SPIS_STATUS_SUCCESS )
	{
		IMG_ASSERT(0);
		return 0;
	}

	return 1;
}

/*!
******************************************************************************

 @Function				TEST_GetTestMessage

******************************************************************************/
int TEST_GetTestMessage( TEST_sDescription	*	psTestDesc,
						 unsigned short		*	pui16CRC	)
{
	TEST_sTestDescPacket	*	psTestDescPkt = (TEST_sTestDescPacket *)g_aui8Scratch;


	if ( SPISRead( &g_sContext.sSPISlave, g_aui8Scratch, sizeof( TEST_sTestDescPacket ), NULL, SPIS_INF_TIMEOUT ) != SPIS_STATUS_SUCCESS )
	{
		IMG_ASSERT(0);
		return 0;
	}

    psTestDesc->eOperation				= (TEST_eOperation)psTestDescPkt->ui8Operation;
	psTestDesc->ui32Mode				= (unsigned long)psTestDescPkt->ui8Mode;
	psTestDesc->ui32Bitrate				= (unsigned long)psTestDescPkt->ui16Bitrate;
	psTestDesc->ui32Length				= (unsigned long)psTestDescPkt->ui16Length;
	psTestDesc->ui32CSActive			= (unsigned long)psTestDescPkt->ui8CSActive;
	psTestDesc->ui32DeviceDMAChannel	= (unsigned long)psTestDescPkt->ui8DMAChannel;

	IMG_ASSERT( pui16CRC );
	*pui16CRC							= psTestDescPkt->ui16CRC;

	return 1;
}

/*!
******************************************************************************

 @Function				TEST_

******************************************************************************/

/*!
******************************************************************************

 @Function				AutoTask

******************************************************************************/
void AutoTask( void )
{
	TEST_sDescription	sTestDesc;
	unsigned short		ui16TxCRC, ui16RxCRC;
	unsigned long		ui32Result;
	KRN_TASKQ_T			sTaskQueue;

	DQ_init( &sTaskQueue );

	// Initialise the random number generat0r
#if defined SPIS_LIBRARY_RAND_AVAILABLE
	srand( (unsigned int)time( NULL ) );
#endif

	// Initialise SPI Slave
	g_sTestProto.ui32BlockIndex = g_ui32BlockNum;
	
	while ( 1 )
	{
		TEST_ConfigureForProtocol();
		// Listen for a test message
		if ( !TEST_GetTestMessage( &sTestDesc, &ui16RxCRC ) )
		{
			break;
		}

		if ( ( sTestDesc.eOperation == OPER_READ ) ||
			 ( sTestDesc.eOperation == OPER_READWRITE ) )
		{
			// Generate data for write part
			TEST_GenerateData( &sTestDesc, &ui16TxCRC );
			// Send ACK with CRC
			if ( !TEST_SendAck( &ui16TxCRC ) )
			{
				break;
			}
		}
		else
		{
			// Send ACK without CRC
			if ( !TEST_SendAck( NULL ) )
			{
				break;
			}
		}

		// Configure test
		if ( !TEST_ConfigureForTest( &sTestDesc ) )
		{
			break;
		}

		// Queue read/write/etc
		switch ( sTestDesc.eOperation )
		{
			case OPER_READ:
			{
				ui32Result = SPISWrite( &g_sContext.sSPISlave, g_aui8RefWriteBuffer, sTestDesc.ui32Length, NULL, SPIS_INF_TIMEOUT );
				break;
			}
			case OPER_WRITE:
			{
				ui32Result = SPISRead( &g_sContext.sSPISlave, g_aui8ReadBuffer, sTestDesc.ui32Length, NULL, SPIS_INF_TIMEOUT );
				break;
			}
			case OPER_READWRITE:
			{
				ui32Result = SPISRead( &g_sContext.sSPISlave, g_aui8ReadBuffer, sTestDesc.ui32Length, NULL, SPIS_INF_TIMEOUT );
				if ( ui32Result != SPIS_STATUS_SUCCESS )
				{
					break;
				}

				ui32Result = SPISWrite( &g_sContext.sSPISlave, g_aui8RefWriteBuffer, sTestDesc.ui32Length, NULL, SPIS_INF_TIMEOUT );
				break;
			}
		}

		IMG_ASSERT( ui32Result == SPIS_STATUS_SUCCESS );

		// If we have received data, check it
		if ( ( sTestDesc.eOperation == OPER_WRITE ) ||
			 ( sTestDesc.eOperation == OPER_READWRITE ) )
		{
			if ( CRCX25( g_aui8ReadBuffer, sTestDesc.ui32Length ) != ui16RxCRC )
			{
				IMG_ASSERT( 0 );
				// For now just ignore this, as the host side will probably be messed up as well.
			}
		}
	}

	// If we get here there has been an error in the program
	IMG_ASSERT( 0 );
}
