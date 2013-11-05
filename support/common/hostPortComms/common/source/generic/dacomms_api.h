/*!
******************************************************************************
 @file   : dacomms_api.h

 @brief  MobileTV DACOMMS API


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
/*
******************************************************************************
 Modifications :- see CVS history

******************************************************************************/

#ifndef __DACOMMS_API_H__
#define __DACOMMS_API_H__

#include "img_defs.h"

#ifdef __cplusplus
extern "C" {
#endif


/*============================================================================
====	D E F I N E S
=============================================================================*/

#define DACOMMS_SPI							(0)
#define DACOMMS_SDIO						(1)
#define DACOMMS_USB							(2)
#define DACOMMS_DASH						(3)
#define DACOMMS_HP							(4)

// SPI/SDIO/USB Version
#define DACOMMS_VARIANT						(DACOMMS_HP)

#define DACOMMS_NUM_HOST_ASYNC_QUEUES		(5)
#define DACOMMS_NUM_GROUPED_BUFFERS			(2)

#define DACOMMS_PAYLOAD_START				(sizeof( DACOMMSMsgHeader ) + sizeof( IMG_UINT8 ) + sizeof( IMG_UINT16 ))
#define DACOMMS_PACKET_PROTOCOL_SIZE		(sizeof( DACOMMSMsgHeader ) + /* Header */			\
											sizeof( IMG_UINT8 ) +		 /* Message type */		\
											sizeof( IMG_UINT16 ) +       /* Payload length */	\
											2 * sizeof( IMG_UINT8 ) +	 /* CRC */				\
											sizeof( DACOMMSMsgTrailer ))	 /* Trailer */


#define DACOMMS_USE_CRC									// Comment this out to disable the use of CRC in transfers

#ifdef DACOMMS_USE_CRC
#define X25_INIT				0xFFFFU						// X25 CRC bits'n'pieces
#define X25_RES					0x1D0FU
#endif

// SDIO specific
#if DACOMMS_VARIANT == DACOMMS_SDIO

	#define SDIO_ES1				(0)
	#define SDIO_ES2				(1)

	#define SDIO_OS_XP				(0)
	#define SDIO_OS_VISTA			(1)

	#define SDIO_HARDWARE			(SDIO_ES1)				// CHANGE THIS TO TARGET HARDWARE
	#define HOST_OS					(SDIO_OS_VISTA)			// CHANGE THIS TO HOST OS
	#define ASYNC_DATA_SIZE			(16384)					// CHANGE TO SUIT APPLICATION, max of 32K

	#if ( HOST_OS == SDIO_OS_XP ) && ( ( SDIO_HARDWARE == SDIO_ES1 ) || ( SDIO_HARDWARE == SDIO_ES2 ) )
		#define SDIO_64BYTE_BUG			(1)
	#endif

	#if ( HOST_OS == SDIO_OS_VISTA) && ( SDIO_HARDWARE == SDIO_ES1 )
		#define SDIO_BYTEBLOCK_BUG		(1)
	#endif

	#if HOST_OS == SDIO_OS_XP
		// There is no block mode in vista, so max transfer is 511 bytes
		#define DACOMMS_MSG_DATA_SIZE	(511 - DACOMMS_PACKET_PROTOCOL_SIZE)
	#elif HOST_OS == SDIO_OS_VISTA
		// We want to make the whole transfer greater than 512 bytes so that vista uses Block Mode
		#define DACOMMS_MSG_DATA_SIZE	(ASYNC_DATA_SIZE - DACOMMS_PACKET_PROTOCOL_SIZE)
	#endif
	#define DACOMMS_ASYNC_DATA_SIZE		(DACOMMS_MSG_DATA_SIZE + DACOMMS_PACKET_PROTOCOL_SIZE)

#elif DACOMMS_VARIANT == DACOMMS_SPI

	#define DACOMMS_MSG_DATA_SIZE		(500)		// For CMD/RSP transactions
	#define DACOMMS_ASYNC_DATA_SIZE		(3 * 1024) // For Async transactions

#elif DACOMMS_VARIANT == DACOMMS_USB

	#define DACOMMS_MSG_DATA_SIZE		(500)		// For CMD/RSP transactions
	#define DACOMMS_ASYNC_DATA_SIZE		(32 * 1024) // For Async transactions

#elif DACOMMS_VARIANT == DACOMMS_DASH

	#define DACOMMS_CMD_DATA_SIZE		(128)
	#define DACOMMS_RSP_DATA_SIZE		(135)
	#define DACOMMS_ASYNC_DATA_SIZE		(128)

	#define DACOMMS_CMD_LENGTH_START	(4)
	#define DACOMMS_RSP_LENGTH_START	(4)
	#define DACOMMS_ASYNC_LENGTH_START	(4)

	#define DACOMMS_CMD_PAYLOAD_START	(sizeof( DACOMMS_sDashCmdRspMsg ))
	#define DACOMMS_RSP_PAYLOAD_START	(sizeof( DACOMMS_sDashCmdRspMsg ))
	#define DACOMMS_ASYNC_PAYLOAD_START	(sizeof( DACOMMS_sDashAsyncMsg ))

	/*
	* Define if using the USB to send data to PC
	* Note that this will DMA data to a channel, which in the FPGA is connected to the USB output, so the
	* data is sent strtaight over the USB. If connected to the PC, the data can be read at the PC.
	*/
	#define DACOMMS_DASH_DATA_OVER_USB
	#define DACOMMS_DASH_USB_SYNC_DATA (0xBEEFA50F)

#elif DACOMMS_VARIANT == DACOMMS_HP

	#define DACOMMS_CMD_DATA_SIZE		(128)
	#define DACOMMS_RSP_ASYNC_DATA_SIZE	(192)

	#define DACOMMS_CMD_PAYLOAD_START		(sizeof( DACOMMS_sHPCmdMsg ))
	#define DACOMMS_RSP_ASYNC_PAYLOAD_START	(sizeof( DACOMMS_sHPRspAsyncMsg ))

#else

	#error Invalid Dacomms Variant Specified!

#endif




/*============================================================================
====	E N U M S
=============================================================================*/
typedef enum
{
	H2D_MSG_CMD					= 0x00,				// Command message (H2D)
	H2D_MSG_REQ_CMD				= 0x01,				// Request for command message
	H2D_MSG_REQ_DATA 			= 0x02,				// Request for data message (rsp or async)
	H2D_MSG_NO_TRANSFER			= 0x03,				// Do not perform a transfer

	D2H_MSG_RSP_START 			= 0x10,				// Response start packet
	D2H_MSG_RSP_DATA 			= 0x11,				// Resposne data packet

	D2H_MSG_ASYNC_INFO_START	= 0x20,				// Async (INFO/EVENT) start packet
	D2H_MSG_ASYNC_DATA			= 0x21,				// Async data packet (D2H)
	D2H_MSG_ASYNC_DATA_START	= 0x30,				// Async (DATA) start packet

	D2H_MSG_ACK					= 0xF0,
	D2H_MSG_NACK				= 0xF1,

} COMMS_eMessageType;

typedef enum
{
	DASH_BUFFER_EMPTY			= 0x00,
	DASH_BUFFER_FULL			= 0x01,
} COMMS_eDashStatus;

typedef enum
{
	COMMS_MSG_CMD				= 0x00,
	COMMS_MSG_RSP				= 0x01,
	COMMS_MSG_ASYNC				= 0x02,
} COMMS_eHPMessageType;

/*============================================================================
====	T Y P E D E F S
=============================================================================*/

/*!
******************************************************************************
 This structure defines the COMMS Start message format
******************************************************************************/

typedef union DACOMMS_tag_sStartMsg
{
	struct
	{
		IMG_UINT8		ui8Header[2];
		IMG_UINT8		ui8MessageType;
		IMG_UINT8		ui8GroupIndex;
		IMG_UINT32		ui32Length;
		IMG_UINT8		ui8Checksum[2];
		IMG_UINT8		ui8Trailer[2];
	} s;
	IMG_UINT8			ui8Data[12];
} DACOMMS_sStartMsg;

#ifdef WIN32
#pragma pack(push, 1)
#endif

typedef union DACOMMS_tag_sAckMsg
{
	struct
	{
		IMG_UINT8		ui8Header[2];
		IMG_UINT8		ui8MessageType;	// Ack or Nack
		IMG_UINT8		ui8Trailer[2];
#if DACOMMS_VARIANT == DACOMMS_SDIO
#if SDIO_HARDWARE == SDIO_ES1
		IMG_UINT8		ui8Padding[3];
#else
		IMG_UINT8		ui8Padding;
#endif
#endif

#if !defined WIN32
	}__attribute__((__packed__)) s;
#else
	} s;
#endif

#if DACOMMS_VARIANT == DACOMMS_SDIO
#if SDIO_HARDWARE == SDIO_ES1
	IMG_UINT8			ui8Data[8];
#else
	IMG_UINT8			ui8Data[6];
#endif
#else
	IMG_UINT8			ui8Data[5];
#endif

#if !defined WIN32
} __attribute__((__packed__)) DACOMMS_sAckMsg;
#else
} DACOMMS_sAckMsg;
#endif

#if defined WIN32
#pragma pack(pop)
#endif



typedef struct DACOMMS_tag_sDashCmdRspMsg
{
	COMMS_eDashStatus	ui8Status;
	IMG_UINT32			ui32Length;
} DACOMMS_sDashCmdRspMsg;

typedef struct DACOMMS_tag_sDashAsyncMsg
{
	COMMS_eDashStatus	ui8Status;
	IMG_UINT32			ui32Length[DACOMMS_NUM_GROUPED_BUFFERS];
} DACOMMS_sDashAsyncMsg;

typedef struct DACOMMS_tag_sHPCmdMsg
{
	IMG_UINT32			ui32Length;
} DACOMMS_sHPCmdMsg;

typedef struct DACOMMS_tag_sHPRspAsyncMsg
{
	COMMS_eHPMessageType	ui8MessageType;
	IMG_UINT32				ui32Length[DACOMMS_NUM_GROUPED_BUFFERS];
} DACOMMS_sHPRspAsyncMsg;

/*============================================================================
====	D A T A
=============================================================================*/
static const IMG_UINT8 DACOMMSMsgHeader[2] = { 0xA5, 0x5A };
static const IMG_UINT8 DACOMMSMsgTrailer[2] = { 0x96, 0x69 };

#ifdef DACOMMS_USE_CRC
// CRC Table
static const unsigned short X25Tab[16] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF
};

#endif

/*============================================================================
====	F U N C T I O N   P R O T O T Y P E S
=============================================================================*/


/*============================================================================
	E N D
=============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* __DACOMMS_API_H__   */

