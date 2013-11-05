/*!
*******************************************************************************
 @file   diseqc_api.h

 @brief  DiSEqC Master API

         This file contains the header file information for the DiSEqC master API.

		 This API allows interaction with the diseqc master controller(s) to 
		 perform the following functions:
		 
		 -Send and receive DiSEqC messages
		 -Assert/clear continuous tones
		 -Send modulated/unmodulated tone bursts

		 The output of the Controller (TX) is either a 22khz square wave or a 
		 logic level to indicate an active tone (the polarity can be configured). 
		 Similarly the input to the controller (RX) should be either a 22khz square 
		 wave or a logic level that indicates an active tone (the polarity can be 
		 configured). Additional analogue circuitry will be required to interface 
		 with a DiSEqC bus.

		 The DiSEqC protocol typically invloves the master sending a message to one 
		 or more slaves, then an individual slave will reply if a response is 
		 requested by the master. This is reflected in the hardware and in this API. 
		 It is however possible to receive unexpected replies and detect potential 
		 contention on the bus to increase flexibility.

		 Ideally the DiSEqC bus configuration will be preset and not present any 
		 contention issues. Where this is not the case the facility to detect 
		 potential contention problems is available through the ability to receive 
		 unexpected replies and to detect receive bit errors. The DiSEqC API does 
		 not provide any high level contention correction code; if required it 
		 will be written by the user of the API.

		 All DiSEqC API callback functions are called in the interrupt context.
		 The code for these callbacks should be short and adhere to interrupt context 
		 code execution rules defined in the MeOS documentation.
		 
		 No DiSEqC API calls should be made while in the interrupt context.

 @author Imagination Technologies

         <b>Copyright 2011 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to the third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, Kings Langley, Hertfordshire,
         WD4 8LZ, U.K.

*******************************************************************************/

#if !defined (__DISEQC_API_H__)
#define __DISEQC_API_H__

/******************************************************************************
**************************** Included header files ****************************
*******************************************************************************/

#include <MeOS.h>


/******************************************************************************
******************************** Enumerations *********************************
*******************************************************************************/


/*!
*******************************************************************************

 These define the status codes for DiSEqC functions/operations.

*******************************************************************************/
typedef enum
{
    /*! Operation completed successfully. */
    DISEQC_STATUS_OK = 0,

	/*! Operation timeout elapsed. Operation not started. */
	DISEQC_STATUS_ERR_TIMEOUT,

	/*! Operation cancelled. Operation not started. */
	DISEQC_STATUS_ERR_CANCEL,

	/*! Error: Receive operation timed out */
	DISEQC_STATUS_ERR_RECEIVE_TIMEOUT,

	/*! Error: Send FIFO empty for too long, the send FIFO empty interrupt was not 
		serviced in time */
	DISEQC_STATUS_ERR_SEND_TIMEOUT,

	/*! Error: Receive overflow, the receive FIFO full interrupt was not serviced 
		in time */
	DISEQC_STATUS_ERR_RECEIVE_OVERFLOW,

	/*! Error: General receive operation failure (bit formation error / parity bit 
		error) - this may indicate contention on the bus */
	DISEQC_STATUS_ERR_RECEIVE,

	/*! Error: The DiSEqC port is already initialised */
	DISEQC_STATUS_ERR_INITIALISED,

	/*! Error: The DiSEqC port is uninitialised */
	DISEQC_STATUS_ERR_UNINITIALISED,

	/*! Error: Invalid block index */
	DISEQC_STATUS_ERR_INVALIDBLOCK,

	/*! Error: Invalid number of I/O blocks */
	DISEQC_STATUS_ERR_NUM_IO_BLOCKS,

	/*! An pointer to an unexpected data buffer was provided but size was set to 0 */
	DISEQC_STATUS_ERR_UNEXPECTED_SIZE,

	/*! Error: No I/O blocks free to use for I/O operation */
	DISEQC_STATUS_ERR_IO_BLOCK,

	/*! Error: Continuous tone is currently asserted. */
	DISEQC_STATUS_ERR_CONTINUOUS_TONE,

}DISEQC_STATUS_T;

/*!
*******************************************************************************

 These define the port state options. The state of a port should be set to
 DISEQC_PORT_STATE_UNINITIALISED prior to a call to DISEQCInit. Other states
 are used internally.

*******************************************************************************/
typedef enum
{
	DISEQC_PORT_STATE_UNINITIALISED = 0,
	DISEQC_PORT_STATE_INITIALISING,
	DISEQC_PORT_STATE_INITIALISED,
	DISEQC_PORT_STATE_DEINITIALISING,
	DISEQC_PORT_STATE_DEINITIALISED,
	DISEQC_PORT_STATE_REINITIALISING
}DISEQC_PORT_STATE_T;

/*!
*******************************************************************************

 These define the receive options. If there will be no contention on the bus and
 slave replies will only appear when expected then DISEQC_RECEIVE_EXPECTED may be used
 to save power.

*******************************************************************************/
typedef enum
{
	/*! Always receive */
	DISEQC_RECEIVE_ALWAYS = 0,

	/*! Only receive when slave message is expected */
	DISEQC_RECEIVE_EXPECTED
}DISEQC_RECEIVE_T;

/*!
*******************************************************************************

 These define the output tone generation and input tone detection. This should be
 set according to the external tone generation/detection circuitry used. The output
 generation tone type and input detection tone type are shared and cannot be
 configured independantly.

*******************************************************************************/
typedef enum
{
	/*! Output on DiSEqC TX is a logic level to represet a tone being active or 
		inactive. Input on DiSEqC RX is a logic level to represet a tone being 
		active or inactive.*/
	DISEQC_TONE_LOGIC = 0,

	/*! Output on DiSEqC tx is a square wave tone. Input on DiSEqC rx is a square 
		wave tone. */
	DISEQC_TONE_SQUARE_WAVE
}DISEQC_TONE_T;

/*!
*******************************************************************************

 These define the polarity levels if logic level tones are to be used. The output 
 and input polarity can be configured independantly.

*******************************************************************************/
typedef enum
{
	/*! Low polarity defines an active tone */
	DISEQC_POLARITY_LOW = 0,

	/*! High polarity defines an active tone */
	DISEQC_POLARITY_HIGH
}DISEQC_POLARITY_T;

/*!
*******************************************************************************

 These define the loopback options. Loopback should be enabled for testing only.
 When loopback is turned on RX is connected to TX and data sent will be immediately 
 received.

*******************************************************************************/
typedef enum
{
	/*! No loopback */
	DISEQC_LOOPBACK_OFF = 0,

	/*! Loopback */
	DISEQC_LOOPBACK_ON
}DISEQC_LOOPBACK_T;

/*!
*******************************************************************************

 These define the synchronous modes of the API functions. When a synchronous API 
 call is made further synchronous + asynchronous API calls will block until the 
 synchronous one has completed.

*******************************************************************************/
typedef enum
{
	/*! Asynchronously queue the operation. The API function will return immediately
		if a different synchronous operation is not currently queued/active. */
	DISEQC_SYNC_ASYNC = 0,
	/*! Perform the operation synchronously. The API function will not return until
		all queued operations and the given operation have completed. */
	DISEQC_SYNC_SYNC
}DISEQC_SYNC_T;


/******************************************************************************
**************************** Constant definitions *****************************
*******************************************************************************/



/******************************************************************************
****************************** Type definitions *******************************
*******************************************************************************/

/*!
*******************************************************************************

 @brief This type defines the completion callback function. This function will be 
		called in the interrupt context and must adhere to the interrupt context 
		execution rules defined in the MeOS documentation.

 \param     *pData				Pointer to received data. IMG_NULL if the operation 
								was not a send message with reply operation.

 \param		ui8BytesReceived	Number of bytes received. This may be greater than the 
								size of the buffer supplied to the operation, in which 
								case the additional bytes at the end have been lost.
								0 if the operation was not a send message with reply 
								operation or Status is not DISEQC_STATUS OK.

 \param		Status				The status of the operation. This should be checked first 
								as the operation may not have succeeded.

 \param		pContext			The context that was provided in the function call that 
								started the operation.
 

*******************************************************************************/

typedef IMG_VOID (*DISEQC_OPERATION_COMPLETE_CALLBACK_T)(IMG_BYTE *pData, IMG_UINT8 ui8BytesReceived, DISEQC_STATUS_T Status, IMG_PVOID pContext);

/*!
*******************************************************************************

 @brief This type defines the unexpected data callback routine used by the DiSEqC 
 device driver. This function will be called in the interrupt context and must 
 adhere to the interrupt context execution rules defined in the MeOS documentation.

 \param     *pData				Pointer to received data. IMG_NULL if the buffer 
								supplied at initialisation was IMG_NULL.

 \param		ui8BytesReceived	Number of bytes received. This may be greater than 
								the size of the unexpected bytes buffer supplied 
								at initialisation, in which case the additional 
								bytes at the end have been lost. 0 if Status
								is not DISEQC_STATUS_SUCCESS.

 \param		Status				The status of the receive operation. This should 
								be checked first as the operation may not have 
								succeeded.
 
*******************************************************************************/
typedef IMG_VOID (*DISEQC_READ_UNEXPECTED_CALLBACK_T)(IMG_BYTE *pData, IMG_UINT8 ui8BytesReceived, DISEQC_STATUS_T Status);

/*!
*******************************************************************************

 @brief This structure defines the DiSEqC receive timings. Each bit received
		consists of 3 'chunks'. A 0 is defined as two active chunks followed by 1
		inactive chunk. A 1 is defined as 1 active chunk followed by 2 inactive
		chunks.

*******************************************************************************/
typedef struct diseqc_receive_timings_t
{
	/*! Tolerance value for tone detection. This is a value 0-1000 that
		indicates how tolerant the detection is for a single active tone cycle.
		The default value is 600. */
	IMG_UINT16	ui16ToneTolerance;
	/*! Number of 22khz cycles that define a receive chunk. The default value is 
		11 (0.5ms). */
	IMG_UINT8	ui8ChunkWidth;
	/*! Minimum number of 22khz tone cycle detections that are required within a 
		received chunk. The default value is 9. */
	IMG_UINT8	ui8MinDetections;
	/*! Number of 22khz cycles that define the end of a received message. The 
		default value is 0x84 (6ms).*/
	IMG_UINT8	ui8PostMessageSilence;
	/*! Maximum number of 22khz cycles a reply can be absent for when a reply is 
		expected. If this timeout is reached the operation will fail with a status 
		of DISEQC_STATUS_ERR_RECEIVE_TIMEOUT. The default value is 0xCE4 (150ms) */
	IMG_UINT16	ui16SlaveResponseTimeout;
}DISEQC_RECEIVE_TIMINGS_T;

/*!
*******************************************************************************

 @brief This structure defines the DiSEqC send timings. Each bit sent consists of
		2 'chunks'. A 0 is defined as a long active chunk followed by a short
		inactive chunk. A 1 is defined as a short active chunk followed by a long
		inactive chunk.

*******************************************************************************/
typedef struct diseqc_send_timings_t
{
	/*! Number of 22khz cycles for a short chunk. The default value is 11 (0.5ms). */
	IMG_UINT8	ui8ShortChunkWidth;
	/*! Number of 22khz cycles for a long chunk. The default value is 22 (1ms). */
	IMG_UINT8	ui8LongChunkWidth;
	/*! Maximum number of 22khz cycles the master FIFO can remain empty while data is 
		still to be sent. If this timeout is reached the operation will fail with a 
		status of DISEQC_STATUS_ERR_SEND_TIMEOUT. The default value is  0x84 (6ms) */
	IMG_UINT16	ui16MasterEmptyTimeout;
	/*! Period of silence to place on the bus after every transaction (22khz cycles). 
		The default value is 0x14A (15ms) */
	IMG_UINT16	ui16PostTransactionSilence;
}DISEQC_SEND_TIMINGS_T;



/*!
*******************************************************************************

 @brief This structure contains initialisation parameters.

*******************************************************************************/
typedef struct diseqc_port_settings_t
{
	/*! DiSEqC Block Index */
	IMG_UINT32							ui32BlockIndex;

	/*! If set to IMG_TRUE override the default 22khz clock frequency with 
		ui32TargetFreq */
	IMG_BOOL							bOverrideFreq;

	/*! Override target frequency in mhz (Q12.20 notation). Ignored if bOverrideFreq 
		is IMG_FALSE. The default value is 0x5A1D (22khz) */
	IMG_UINT32							ui32TargetFreq;

	/*! Receive timings. IMG_NULL may be specified to use the defaults */
	DISEQC_RECEIVE_TIMINGS_T			*psReceiveTimings;

	/*! Send timings. IMG_NULL may be specified to use the defaults. */
	DISEQC_SEND_TIMINGS_T				*psSendTimings;

	/*! Receive detection */
	DISEQC_RECEIVE_T					Receive;

	/*! Tone generation/detection method for input and output */
	DISEQC_TONE_T						Tone;

	/*! Polarity received on DiSEqC rx that defines an active tone. If Tone is
		set to DISEQC_TONE_SQUARE_WAVE this member is not used. */
	DISEQC_POLARITY_T					InputPolarity;

	/*! Polarity generated on DiSEqC tx that defines an active tone. If Tone is
		set to DISEQC_TONE_SQUARE_WAVE this member is not used. */
	DISEQC_POLARITY_T					OutputPolarity;

	/*! Loopback */
	DISEQC_LOOPBACK_T					Loopback;

	/*! Unexpected data callback. If set to IMG_NULL unexpected bytes will be 
		discarded if detected */
	DISEQC_READ_UNEXPECTED_CALLBACK_T	pfnReadUnexpectedCallback;

	/*! Unexpected data buffer. Not used if pfnReadUnexpectedCallback is IMG_NULL.
		Can be set to IMG_NULL if notification of unexpected data is desired but
		the content of the message is not. */
	IMG_BYTE							*pui8UnexpectedData;

	/*! Size of the unexpected data buffer. If an unexpected message is received that
		is larger than the buffer the bytes at the end will be lost. Not used if 
		pfnReadUnexpectedCallback is IMG_NULL. */
	IMG_UINT8							ui8UnexpectedDataMaxSize;


}DISEQC_PORT_SETTINGS_T;

/*!
*******************************************************************************

 @brief This structure defines the DiSEqC I/O operation block object. These objects 
		are used internally by the driver.

*******************************************************************************/

typedef struct diseqc_io_block_t
{
	/*! Used internally */
	QIO_IOCB_T								iocb;

	/*! Used internally */
	DISEQC_OPERATION_COMPLETE_CALLBACK_T	pfnCompleteCallback;

	/*! Used internally */
	IMG_PVOID								pCompleteCallbackContext;

	/*! Used internally */
	IMG_BYTE								*pui8RecvBuf;

	/*! Used internally */
	IMG_UINT8								ui8RecvBufSize;

	/*! Used internally */
	IMG_UINT8								*pui8Received;

	/*! Used internally */
	DISEQC_SYNC_T							Sync;

	/*! Used internally */
	KRN_SEMAPHORE_T							Sema;

}DISEQC_IO_BLOCK_T;

/*!
*******************************************************************************

 @brief This structure defines the DiSEqC port object. The State member should
		be set to DISEQC_PORT_STATE_UNINITIALISED prior to a DISEQCInit call. 

*******************************************************************************/
typedef struct diseqc_port_t
{
	/*! Port state */
	DISEQC_PORT_STATE_T					State;

	/*! Used internally */
	DISEQC_PORT_SETTINGS_T				*psSettings;
	
    /*! Used internally */
    QIO_DEVICE_T						sDevice;

	/*! Used internally */
	KRN_SEMAPHORE_T						Sema;

	/*! Used internally */
	DISEQC_READ_UNEXPECTED_CALLBACK_T	pfnReadUnexpectedCallback;

	/*! Used internally */
	IMG_BYTE							*pui8UnexpectedData;

	/*! Used internally */
	IMG_UINT8							ui8UnexpectedDataMaxSize;

	/*! Used internally */
	KRN_POOL_T							IOBlockPool;

	/*! Used internally */
	IMG_UINT32							ui32PostTransactionSilence;

	/*! Used internally */
	IMG_UINT32							ui32SlaveResponseTimeout;

	/*! Used internally */
	IMG_UINT32							ui32MasterEmptyTimeout;

	/*! Used internally */
	IMG_UINT32							ui32PostSlaveMessageSilence;

	/*! Used internally */
	QIO_IOPARS_T						*psCurOperation;

	/*! Used internally */
	QIO_IOPARS_T						*psDelayOperation;

	/*! Used internally */
	IMG_BOOL							bContinuousTone;

	/*! Used internally */
	IMG_UINT8							ui8Received;

	/*! Used internally */
	IMG_BOOL							bSilence;

	/*! Used internally */
	DISEQC_STATUS_T						Status;

	/*! Used internally */
	IMG_UINT8							ui8TotalReceived;

	/*! Used internally */
	IMG_BOOL							bMasterData;

}DISEQC_PORT_T;




/******************************************************************************
**************************** Function definitions *****************************
*******************************************************************************/

/*!
*******************************************************************************

 @Function              @DISEQCInit

 <b>Description:</b>\n
 This function is used to initialise a DiSEqC port. This must be used before any
 other DISEQC API calls are made using the port descriptor.

 \param     *psPort				Pointer to port descriptor. The state member 
								should be set to DISEQC_PORT_STATE_UNINITIALISED. 
								The port descriptor must be preserved until a call 
								is made to DISEQCDeinit with the port descriptor.

 \param     *psSettings			Pointer to initialisation settings.

 \param		*pasIOBlocks		Pointer to array of DISEQC_IO_BLOCK_T structures 
								for use with operations. The size of this array 
								should be sufficient to accomodate the maximum 
								simultaneous number of operations that may be 
								queued by the software. This array must be preserved
								until a call is made to DISEQCDeinit with the port
								descriptor.

 \param		ui32NumIOBlocks		Number of DISEQC_IO_BLOCK_T structures in the array 
								pointed to by *pasIOBlocks. Must be >=1

 \return                        This function returns as follows:\n
								::DISEQC_STATUS_OK Operation completed successfully.\n
								::DISEQC_STATUS_ERR_INITIALISED Port descriptor is already initialised.\n
								::DISEQC_STATUS_ERR_INVALIDBLOCK Invalid block index.\n
                               

*******************************************************************************/
DISEQC_STATUS_T DISEQCInit
(
    DISEQC_PORT_T			*psPort,
    DISEQC_PORT_SETTINGS_T	*psSettings,
	DISEQC_IO_BLOCK_T		*pasIOBlocks, 
    IMG_UINT32				ui32NumIOBlocks
);


/*!
*******************************************************************************

 @Function              @DISEQCStartContinuousTone

 <b>Description:</b>\n
 This function is used to start a continuous tone on the given DiSEqC port.

 The operation is completed when the continuous tone has begun.

 Following a call to this function a DISEQCEndContinuousTone operation must be used
 before a tone burst or DiSEqC message can be sent.

 \param     *psPort				Pointer to port descriptor.
 
 \param		Sync				Synchronous mode for the operation.

 \param		i32Timeout			Timeout for the operation (in MeOS scheduler ticks). 
								If the operation cannot be started within the timeout 
								it will not take place. Use KRN_INFWAIT for an infinite 
								timeout.

 \param		pfnCompleteFunction	Complete callback. Set to IMG_NULL if completion 
								notification is not required.

 \param		pCompleteContext	Context to pass to the complete callback.


 \return                        This function returns as follows:\n
								::DISEQC_STATUS_OK Operation completed successfully.\n
								::DISEQC_STATUS_ERR_UNINITIALISED Port descriptor not initialised.\n
								::DISEQC_STATUS_ERR_IO_BLOCK No I/O blocks available.\n
								::DISEQC_STATUS_ERR_CANCEL The operation was cancelled.\n
								::DISEQC_STATUS_ERR_TIMEOUT The operation timeout was reached.\n

*******************************************************************************/
DISEQC_STATUS_T DISEQCStartContinuousTone
(
	DISEQC_PORT_T							*psPort,
	DISEQC_SYNC_T							Sync,
	IMG_INT32								i32Timeout,
	DISEQC_OPERATION_COMPLETE_CALLBACK_T	pfnCompleteFunction,
	IMG_PVOID								pCompleteContext
);

/*!
*******************************************************************************

 @Function              @DISEQCEndContinuousTone

 <b>Description:</b>\n
 This function is used to end a continuous tone on the given DiSEqC port.

 The operation is completed when the continuous tone has been stopped and the post
 transaction silence time has elapsed.

 \param     *psPort				Pointer to port descriptor.
 
 \param		Sync				Synchronous mode for the operation.

 \param		i32Timeout			Timeout for the operation (in MeOS scheduler ticks). 
								If the operation cannot be started within the timeout 
								it will not take place. Use KRN_INFWAIT for an infinite 
								timeout.

 \param		pfnCompleteFunction	Complete callback. Set to IMG_NULL if completion 
								notification is not required.

 \param		pCompleteContext	Context to pass to the complete callback.


 \return                        This function returns as follows:\n
								::DISEQC_STATUS_OK Operation completed successfully.\n
								::DISEQC_STATUS_ERR_UNINITIALISED Port descriptor not initialised.\n
								::DISEQC_STATUS_ERR_IO_BLOCK No I/O blocks available.\n
								::DISEQC_STATUS_ERR_CANCEL The operation was cancelled.\n
								::DISEQC_STATUS_ERR_TIMEOUT The operation timeout was reached.\n

*******************************************************************************/
DISEQC_STATUS_T DISEQCEndContinuousTone
(
	DISEQC_PORT_T							*psPort,
	DISEQC_SYNC_T							Sync,
	IMG_INT32								i32Timeout,
	DISEQC_OPERATION_COMPLETE_CALLBACK_T	pfnCompleteFunction,
	IMG_PVOID								pCompleteContext
);

/*!
*******************************************************************************

 @Function              @DISEQCToneBurstA

 <b>Description:</b>\n
 This function is used to send a tone burst A (unmodulated) on the given DiSEqC 
 port.

 The operation is completed when the tone burst has been sent and the post
 transaction silence time has elapsed.

 \param     *psPort				Pointer to port descriptor.
 
 \param		Sync				Synchronous mode for the operation.

 \param		i32Timeout			Timeout for the operation (in MeOS scheduler ticks). 
								If the operation cannot be started within the timeout 
								it will not take place. Use KRN_INFWAIT for an infinite 
								timeout.

 \param		pfnCompleteFunction	Complete callback. Set to IMG_NULL if completion 
								notification is not required.

 \param		pCompleteContext	Context to pass to the complete callback.


 \return                        This function returns as follows:\n
								::DISEQC_STATUS_OK Operation completed successfully.\n
								::DISEQC_STATUS_ERR_UNINITIALISED Port descriptor not initialised.\n
								::DISEQC_STATUS_ERR_IO_BLOCK No I/O blocks available.\n
								::DISEQC_STATUS_ERR_CONTINUOUS_TONE The last operation queued was a
								continuous tone. This tone must be ended before a tone burst can be sent.\n
								::DISEQC_STATUS_ERR_CANCEL The operation was cancelled.\n
								::DISEQC_STATUS_ERR_TIMEOUT The operation timeout was reached.\n


*******************************************************************************/
DISEQC_STATUS_T DISEQCToneBurstA
(
	DISEQC_PORT_T							*psPort,
	DISEQC_SYNC_T							Sync,
	IMG_INT32								i32Timeout,
	DISEQC_OPERATION_COMPLETE_CALLBACK_T	pfnCompleteFunction,
	IMG_PVOID								pCompleteContext
);

/*!
*******************************************************************************

 @Function              @DISEQCToneBurstB

 <b>Description:</b>\n
 This function is used to send a tone burst B (modulated) on the given DiSEqC port.

 The operation is completed when the tone burst has been sent and the post
 transaction silence time has elapsed.

 \param     *psPort				Pointer to port descriptor.
 
 \param		Sync				Synchronous mode for the operation.

 \param		i32Timeout			Timeout for the operation (in MeOS scheduler ticks). 
								If the operation cannot be started within the timeout 
								it will not take place. Use KRN_INFWAIT for an infinite 
								timeout.

 \param		pfnCompleteFunction	Complete callback. Set to IMG_NULL if completion 
								notification is not required.

 \param		pCompleteContext	Context to pass to the complete callback.


 \return                        This function returns as follows:\n
								::DISEQC_STATUS_OK Operation completed successfully.\n
								::DISEQC_STATUS_ERR_UNINITIALISED Port descriptor not initialised.\n
								::DISEQC_STATUS_ERR_IO_BLOCK No I/O blocks available.\n
								::DISEQC_STATUS_ERR_CONTINUOUS_TONE The last operation queued was a
								continuous tone. This tone must be ended before a tone burst can be sent.\n
								::DISEQC_STATUS_ERR_CANCEL The operation was cancelled.\n
								::DISEQC_STATUS_ERR_TIMEOUT The operation timeout was reached.\n

*******************************************************************************/
DISEQC_STATUS_T	DISEQCToneBurstB
(
	DISEQC_PORT_T							*psPort,
	DISEQC_SYNC_T							Sync,
	IMG_INT32								i32Timeout,
	DISEQC_OPERATION_COMPLETE_CALLBACK_T	pfnCompleteFunction,
	IMG_PVOID								pCompleteContext
);


/*!
*******************************************************************************

 @Function              @DISEQCSendMessage

 <b>Description:</b>\n
 This function is used to send a DiSEqC message on the given port.

 The operation is completed when the message has been sent, an optional reply
 has been received and the post transaction silence time has elapsed.


 \param     *psPort				Pointer to port descriptor.

 \param     *pui8SendBuf		Pointer to data to be sent.

 \param		ui8SendSize			Length of data to be sent. >=1

 \param		*pui8RecvBuf		Pointer to receive buffer. Set to IMG_NULL for a
								send only operation.

 \param		ui8MaxRecvSize		Maximum length of data that can be received.

 \param		*pui8Received		Pointer to storage for the number of bytes received.
								This will be set by the API when the operation has
								completed.

 \param		Sync				Synchronous mode for the operation.

 \param		i32Timeout			Timeout for the operation (in MeOS scheduler ticks).
								If the operation cannot be started within the timeout
								it will not take place. Use KRN_INFWAIT for an infinite
								timeout.

 \param		pfnCompleteFunction	Complete callback. Set to IMG_NULL if completion
								notification is not required.

 \param		pCompleteContext	Context to pass to the complete callback.


 \return                        This function returns as follows:\n
								::DISEQC_STATUS_OK Operation completed successfully.\n
								::DISEQC_STATUS_ERR_UNINITIALISED Port descriptor not initialised.\n
								::DISEQC_STATUS_ERR_IO_BLOCK No I/O blocks available.\n
								::DISEQC_STATUS_ERR_CONTINUOUS_TONE The last operation queued was a
								continuous tone. This tone must be ended before a message can be sent.\n
								::DISEQC_STATUS_ERR_CANCEL The operation was cancelled.\n
								::DISEQC_STATUS_ERR_TIMEOUT The operation timeout was reached.\n
								::DISEQC_STATUS_ERR_RECEIVE_TIMEOUT Receive timeout.\n
								::DISEQC_STATUS_ERR_SEND_TIMEOUT Master FIFO empty for too long.\n
								::DISEQC_STATUS_ERR_RECEIVE_OVERFLOW Slave FIFO full for too long.\n
								::DISEQC_STATUS_ERR_RECEIVE receive error.\n
		                        

*******************************************************************************/
DISEQC_STATUS_T DISEQCSendMessage
(
    DISEQC_PORT_T							*psPort,
	IMG_BYTE								*pui8SendBuf,
	IMG_UINT8								ui8SendSize,
	IMG_BYTE								*pui8RecvBuf,
	IMG_UINT8								ui8MaxRecvSize,
	IMG_UINT8								*pui8Received,
	DISEQC_SYNC_T							Sync,
	IMG_INT32								i32Timeout,
	DISEQC_OPERATION_COMPLETE_CALLBACK_T	pfnCompleteFunction,
	IMG_PVOID								pCompleteContext
);


/*!
*******************************************************************************

 @Function              @DISEQCCancelAll

 <b>Description:</b>\n
 This function is used to cancel all pending operations for a given DiSEqC port.

 \param     *psPort				Pointer to port descriptor.

 \return                        This function returns as follows:\n
								::DISEQC_STATUS_OK Operation completed successfully.\n
								::DISEQC_STATUS_ERR_UNINITIALISED Port descriptor not initialised.\n
                               

*******************************************************************************/
DISEQC_STATUS_T DISEQCCancelAll
(
	DISEQC_PORT_T	*psPort
);

/*!
*******************************************************************************

 @Function              @DISEQCDeinit

 <b>Description:</b>\n
 This function is used to deinitialise a DiSEqC port.

 \param     *psPort				Pointer to port descriptor.

 \return                        This function returns as follows:\n
								::DISEQC_STATUS_OK Operation completed successfully.\n
                                ::DISEQC_STATUS_ERR_UNINITIALISED Port descriptor not initialised.\n
								::DISEQC_STATUS_ERR_IO_BLOCK No I/O blocks available.\n

*******************************************************************************/
DISEQC_STATUS_T DISEQCDeinit
(
	DISEQC_PORT_T			*psPort
);

/******************************************************************************
******************************** End Functions ********************************
*******************************************************************************/

#endif /* __DISEQC_API_H__ */
