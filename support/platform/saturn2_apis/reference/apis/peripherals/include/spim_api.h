/*!
*******************************************************************************
 @file   spim_api.h

 @brief  Serial Peripheral Interface Master API

         This file contains the header file information for the Serial
         Peripheral Interface Master (SPIM) API.

 @author Imagination Technologies

         <b>Copyright 2006-2007 by Imagination Technologies Limited.</b>\n
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
/*
*******************************************************************************
 Modifications :-

 $Log: spim_api.h,v $

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 

  --- Revision Logs Removed --- 


*******************************************************************************/

#ifndef __SPIM_API_H__
#define __SPIM_API_H__

#include <MeOS.h>

#include "img_defs.h"
#include "ioblock_defs.h"
#include "gdma_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
====	D E F I N E S
=============================================================================*/

/*! Number of chip select lines supported by the SPI master port. */
#define SPIM_NUM_PORTS_PER_BLOCK			( SPIM_DUMMY_CS )
/*! Maximum transfer size (in bytes) for the SPI master port. */
#define SPIM_MAX_TRANSFER_BYTES				( 65536 )

/*============================================================================
====	E N U M S
=============================================================================*/

/*!
*******************************************************************************
 This type defines the SPIM initialisation function return value.
*******************************************************************************/
typedef enum
{
    /*! Initialisation parameters are valid. */
    SPIM_OK = 0,
    /*! DMA channel parameters are invalid. */
    SPIM_INVALID_DMA_CHANNELS,
    /*! Mode parameter is invalid. */
    SPIM_INVALID_SPI_MODE,
    /*! Chip select idle level is invalid. */
    SPIM_INVALID_CS_IDLE_LEVEL,
    /*! Data idle level is invalid. */
    SPIM_INVALID_DATA_IDLE_LEVEL,
    /*! Chip select line parameter is invalid. */
    SPIM_INVALID_CS_LINE,
    /*! Transfer size parameter is invalid. */
    SPIM_INVALID_SIZE,
    /*! Read/write parameter is invalid. */
    SPIM_INVALID_READ_WRITE,
    /*! Continue parameter is invalid. */
    SPIM_INVALID_CONTINUE,
    /*! Compare equal parameter is invalid. */
    SPIM_INVALID_COMPARE_EQ,
	/*! Invalid block index */
	SPIM_INVALID_BLOCK_INDEX
	
} SPIM_eReturn;

/*!
*******************************************************************************
 This type defines the SPI Mode.
*******************************************************************************/
typedef enum
{
    /*! Mode 0 (clock idle low, data valid on first clock transition). */
	SPIM_MODE_0 = 0,
    /*! Mode 1 (clock idle low, data valid on second clock transition). */
	SPIM_MODE_1,
    /*! Mode 2 (clock idle high, data valid on first clock transition). */
	SPIM_MODE_2,
    /*! Mode 3 (clock idle high, data valid on second clock transition). */
	SPIM_MODE_3
	
} SPIM_eMode;

/*!
*******************************************************************************
 This type defines the SPIM device numbers (chip select lines).
*******************************************************************************/
typedef enum
{
    /*! Device 0 (CS0). */
	SPIM_DEVICE0 = 0,
    /*! Device 1 (CS1). */
	SPIM_DEVICE1,
    /*! Device 2 (CS2). */
	SPIM_DEVICE2,
    /*! Dummy chip select. */
	SPIM_DUMMY_CS
	
} SPIM_eDevice;


/*============================================================================
====	T Y P E D E F S
=============================================================================*/

/*!
*******************************************************************************
 @brief This structure defines communication parameters for a given slave device
*******************************************************************************/
typedef struct spim_device_param_t
{
    /*! Bit rate value.\n
        Is calculated with the formula ui8BitRate = ((512 * desired Bit Clock rate) / Input clock freq) .*/
	IMG_UINT8		ui8BitRate;
    /*! Chip select set up time.\n
    	Time taken between chip select going active and activity occurring on the clock.\n
    	Calculated by dividing the desired set up time in ns by the Input clock period. (setup time / Input clock freq) */
	IMG_UINT8		ui8CSSetup;
    /*! Chip select hold time.\n
    	Time after the last clock pulse before chip select goes inactive.\n
    	Calculated by dividing the desired hold time in ns by the Input clock period (hold time / Input clock freq). */
	IMG_UINT8		ui8CSHold;
    /*! Chip select delay time (CS minimum inactive time).\n
    	Minimum time after chip select goes inactive before chip select can go active again.\n
    	Calculated by dividing the desired delay time in ns by the Input clock period (delay time / Input clock freq). */
	IMG_UINT8		ui8CSDelay;
    /*! SPI Mode. */
	SPIM_eMode		eSPIMode;
    /*! Chip select idle level (0=low, 1=high, Others=invalid). */
    IMG_UINT32		ui32CSIdleLevel;
    /*! Data idle level (0=low, 1=high, Others=invalid). */
    IMG_UINT32		ui32DataIdleLevel;
    	
} SPIM_sDeviceParam;

/*!
*******************************************************************************
 @brief This structure contains initialisation parameters.
*******************************************************************************/
typedef struct spim_init_param_t
{
    /*! Parameters for chip select 0. */
	SPIM_sDeviceParam	sDev0Param;
    /*! Parameters for chip select 1. */
	SPIM_sDeviceParam	sDev1Param;
    /*! Parameters for chip select 2. */
	SPIM_sDeviceParam	sDev2Param;
    /*! Input/Output DMA channel */
	img_uint32			ui32DMAChannel;
	/* Block index */
	img_uint32			ui32BlockIndex;
	/* Bypass QIO */
	img_bool			bBypassQIO;
	/* Bypass DMA when not using QIO */
	img_bool			bBypassDMA;
	
} SPIM_sInitParam;

/*!
*******************************************************************************
 @brief This structure defines the SPIM block object - NO USER SERVICABLE PARTS
*******************************************************************************/
typedef struct spim_block_tag 
{
	/*! Is API initialised? */
	IMG_BOOL				bInitialised;
	
    /*! QIO device. */
	QIO_DEVICE_T			sDevice;
	
	/*! Pointer to current transfer */
	QIO_IOPARS_T		*	psIOPars;
	/*! Device state */
	img_uint32				ui32State;
	/*! DMA channel */
	img_uint32				ui32DMAChannel;
	/* GDMA context */
	GDMA_sContext			sDMAContext;
	/* GDMA Transfer Object */	
	GDMA_sTransferObject	sDMATransfer[2];
	/* QIO Bypass */
	img_bool				bBypassQIO;
	/* QIO & DMA Bypass */
	img_bool				bBypassDMA;
	/* SPIM Interrupt Handled */
	img_bool				bSPIMInterruptHandled;

} SPIM_sBlock;

/*!
*******************************************************************************
 @brief This structure contains parameters that describe an SPIM operation.
*******************************************************************************/
typedef struct spim_buf_t
{
    /*! The buffer to read from or write to.\n
       	Set by the user for a standard operation.\n
       	Not used for a compare operation.*/
    IMG_UINT8  *	pui8Buffer;
    /*! Number of bytes to read/write. Valid range is 0 to 65536 bytes.\n
    	Note that a given size of 0 will transfer a maximum size of 65536 bytes.\n
    	Set by the user for a standard operation.\n
       	Used but not set by the user for a compare operation.*/
    IMG_UINT32		ui32Size;
    /*! Read/write select. TRUE for read, FALSE for write, Others - invalid.\n
    	Set by the user for a standard operation.\n
       	Used but not set by the user for a compare operation.*/
    IMG_INT32		i32Read;
    /*! Continue select.\n
        (Selects whether or not chip select is kept active after the operation).\n
        0 - Non-continous, 1 - Continuous, Others - invalid.\n
		Set by the user for a standard operation.\n
		Set by the user for a compare operation.*/
    IMG_INT32		i32Cont;
	/*! ByteDelay select.\n
		(Selects whether or not a delay is inserted between bytes).\n
		0 - Minimum inter-byte delay
		1 - Inter-byte delay of (cs_hold value / master_clk half period) * master_clk.
		Note that this is only applicable for bytes within a transaction if the continue bit
		is set at the end of the transaction. (refer spi_reg3[29] description in SPI Master TRM) */
	IMG_INT32		i32InterByteDelay;
	/*! Chip select line (which slave to address).\n
		Set by the user for a standard operation.\n
		Set by the user for a compare operation.*/
    SPIM_eDevice	eChipSelect;
    /*! Value to compare with incoming data when 'Compare Data' is used.\n
		Not used for a standard operation.\n
		Set by the user for a compare operation.*/
    IMG_UINT8		ui8CmpValue;
    /*! Compare data mask.
    	Applied to both the received byte and cmpValue for comparison.\n
		Not used for a standard operation.\n
		Set by the user for a compare operation.*/
    IMG_UINT8		ui8CmpMask;
    /*! Compare data equal/not equal select:\n
        1 - Flag set when incoming data is equal to compare data value.\n
        0 - Flag set when incoming data is not equal to compare data value.\n
        Others - invalid.\n
		Not used for a standard operation.\n
		Set by the user for a compare operation.*/
    IMG_INT			iCmpEq;
    /*! Private.\n
	Used but not set by the user for a standard operation.\n
	Used but not set by the user for a compare operation.*/
    IMG_INT32		i32CmpData;
    
} SPIM_sBuffer;


/*============================================================================
====	F U N C T I O N   P R O T O T Y P E S
=============================================================================*/

/*!
*******************************************************************************

 @Function				@SPIMDefine

 @Description
 This function defines an instance of a SPI Master block.

*******************************************************************************/
img_void SPIMDefine(	ioblock_sBlockDescriptor	*	psBlockDescriptor	);


/*!
*******************************************************************************

 @Function              @SPIMInit

 @Description         
 This function is used to initialise the SPI Master module.

 It must be called before any other SPI Master API function. It is not
 necessary to call it again unless any of the parameters set up in SPIMInit()
 are to be altered.

 The function initialises the port, ready for a read or write transaction
 using any of the available chip select (CS) lines.\n
 Separate timing parameters may be specified for each CS line and the DMA
 channels to be used for input and output data are configured.\n
 The input and output DMA channels must not be the same.

 @Input     psBlock             Pointer to block descriptor.

 @Input     psInitParam         Pointer to initialisation parameters.

 @Return    SPIM_eReturn        SPIM_OK if initialisation parameters are valid
                                SPIM_ERR otherwise

*******************************************************************************/
SPIM_eReturn SPIMInit
(
    SPIM_sBlock			*	psBlock,
    SPIM_sInitParam		*	psInitParam
);


/*!
*******************************************************************************

 @Function              @SPIMDeinit
 
 @Description
 This function is used to deinitialise the API and should be used as part of 
 a safe system shutdown.
 
  @Input    psBlock             Pointer to block descriptor.
  
  @Return	SPIM_eReturn		SPIM_OK if the block pointer is valid, or
  								SPIM_ERR otherwise.

*******************************************************************************/
SPIM_eReturn SPIMDeinit
(
    SPIM_sBlock			*	psBlock
);


/*!
*******************************************************************************

 @Function              @SPIMReadWrite

 @Description         
 This function is used to perform an SPI master transaction.

 The transaction can involve up to three stages, described by the inputs a, b
 and c, which are pointers to SPIM_BUF_T structures containing parameters
 for each operation.

 'a' describes the first standard read or write operation.

 If required, this can be followed immediately by another read or write
 operation described by 'b'. If this second read/write is not required, b
 should be set to NULL.

 If the SPIM's compare data functionality is to be used, then this part of the
 transaction is described in 'c'.

 The SPIM API and device driver treats the compare data detection as another
 operation, which is why the calling function defines it using a third
 SPIM_BUF_T pointer. The compare date operation will always precede the first
 standard read/write described in 'a'. The compare data operation involves
 reading in a series of data bytes from the slave (which are discarded) until
 it receives a byte that is valid for compare data detection (as defined by
 the parameters in c). At this point it starts the operation defined by 'a'

 If the compare data operation is not required, c should be set to NULL.

 Of the parameters contained in the SPIM_BUF_T structure, cmpValue, cmpMask
 and cmpEq define the conditions for detection of a 'compare data byte'.
 cmpValue is the value that the SPIM compares with each byte received in a
 compare data operation. cmpMask is the bit mask applied to both the received
 byte and cmpValue when the comparison is made. cmpEq determines whether a
 detection occurs when the two bytes are equal or when they are not equal.

 When a standard SPIM operation (as described by a and/or b) is carried out,
 the SPIM sets one of the three CS lines (decided by 'chipSelect') to active
 to begin the operation and transfers 'size' bytes in and out of the SPIM.

 This driver does not support simultaneous input and output of data, so for
 reads no data is written (or any that may inadvertantly be written should
 be ignored by the slave) and for writes, any received data is ignored.

 The 'chipSelect' parameter determines which of the three SPIM_DEVICE_PARAM_T
 parameter settings (set in the SPIMInit function) will be used.

 The 'read' parameter determines whether an operation is a read or write.

 The 'buf' parameter points to the first term in the buffer array where,
 for reads, received data will be stored and, for writes, data will be written
 from. At the end of the operation, the CS line will go inactive unless 'cont'
 is set to TRUE, in which case the CS line will remain active for the next operation.

 @Input     *psBlock            Pointer to block descriptor.

 @Input     *psA                Pointer to parameters for first read or write
                                operation in the transaction (excluding compare
                                data operation).

 @Input     *psB                Pointer to parameters for second read or write
                                operation in the transaction (NULL if not required).

 @Input     *psC                Pointer to parameters for compare data operation
                                (NULL if not required).

 @Return	SPIM_eReturn        SPIM_OK if initialisation parameters are valid
                                SPIM_INVALID_<type> if <type> parameter is not valid

*******************************************************************************/
SPIM_eReturn SPIMReadWrite
(
    SPIM_sBlock		*	psBlock,
    SPIM_sBuffer	*	psA,
    SPIM_sBuffer	*	psB,
    SPIM_sBuffer	*	psC
);


extern ioblock_sBlockDescriptor	IMG_asSPIMBlock[];


/*============================================================================
	E N D
=============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* __SPIM_API_H__ */
