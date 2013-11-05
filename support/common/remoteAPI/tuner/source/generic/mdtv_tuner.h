/*!
******************************************************************************
 @file   mdtv_tuner.h

 @brief  MobileTV Remote API ATSC Message Definitions

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

******************************************************************************/

#ifndef _MDTV_TUNER_H_
#define _MDTV_TUNER_H_

#include "mdtv.h"

/*****************************************************************************

 This type defines the buffer sizes for the command, response and async messages.
 When adding new remote function definitions to this file, make sure that these
 buffers are appropriately sized to hold them.

******************************************************************************/

// application resources
#define TUNER_COMMBUFFMAXBYTES		100
#define TUNER_RESBUFFMAXBYTES		64
#define TUNER_ASYNCBUFFMAXBYTES		64

/*****************************************************************************

 This type defines the possible Function ID's avaiable in the DVBT Remote API

******************************************************************************/

typedef enum {
	TUNER_COMM_REGISTER_CTRL_STRUCTS = 0,
	TUNER_RESP_REGISTER_CTRL_STRUCTS = 1,
	TUNER_COMM_SETUP_TUNER_CTRL = 2,
	TUNER_RESP_SETUP_TUNER_CTRL = 3,
	TUNER_COMM_SETUP_SCP_CONFIG = 4,
	TUNER_RESP_SETUP_SCP_CONFIG = 5
} MDTV_TUNER_FUNCTION_ID_E;

/*****************************************************************************

 These definitions define the byte placement and the size of the fields in the
 variable length data segment of the Remote API, for each function.

******************************************************************************/

//----------------------------------------------------------------------------
// Function              TUNER_RegisterCtrlStructs
// Direction             Command

//all data
#define TUNER_COMM_REGISTER_CTRL_STRUCTS_SIZE					(0)

//----------------------------------------------------------------------------
// Function              TUNER_RegisterCtrlStructs
// Direction             Response

//return
#define TUNER_RESP_REGISTER_CTRL_STRUCTS_RETURN_INDEX			(MDTV_HEADER_SIZE)
#define TUNER_RESP_REGISTER_CTRL_STRUCTS_RETURN_SIZE			(4)

//all data
#define TUNER_RESP_REGISTER_CTRL_STRUCTS_SIZE					(TUNER_RESP_REGISTER_CTRL_STRUCTS_RETURN_INDEX + TUNER_RESP_REGISTER_CTRL_STRUCTS_RETURN_SIZE - MDTV_HEADER_SIZE)

//----------------------------------------------------------------------------
// Function              TUNER_setupTunerControl
// Direction             Command

//complexIF
#define TUNER_COMM_SETUP_TUNER_CTRL_I32COMPLEXIF_INDEX			(MDTV_HEADER_SIZE)
#define TUNER_COMM_SETUP_TUNER_CTRL_I32COMPLEXIF_SIZE			(4)

//frequencyIF
#define TUNER_COMM_SETUP_TUNER_CTRL_I32FREQUENCYIF_INDEX		(TUNER_COMM_SETUP_TUNER_CTRL_I32COMPLEXIF_INDEX + TUNER_COMM_SETUP_TUNER_CTRL_I32COMPLEXIF_SIZE)
#define TUNER_COMM_SETUP_TUNER_CTRL_I32FREQUENCYIF_SIZE			(4)

//spectrumInverted
#define TUNER_COMM_SETUP_TUNER_CTRL_I32SPECTRUMINVERTED_INDEX	(TUNER_COMM_SETUP_TUNER_CTRL_I32FREQUENCYIF_INDEX + TUNER_COMM_SETUP_TUNER_CTRL_I32FREQUENCYIF_SIZE)
#define TUNER_COMM_SETUP_TUNER_CTRL_I32SPECTRUMINVERTED_SIZE	(4)

//PLLStepSize
#define TUNER_COMM_SETUP_TUNER_CTRL_I32PLLSTEPSIZE_INDEX		(TUNER_COMM_SETUP_TUNER_CTRL_I32SPECTRUMINVERTED_INDEX + TUNER_COMM_SETUP_TUNER_CTRL_I32SPECTRUMINVERTED_SIZE)
#define TUNER_COMM_SETUP_TUNER_CTRL_I32PLLSTEPSIZE_SIZE			(4)

//all data
#define TUNER_COMM_SETUP_TUNER_CTRL_SIZE						(TUNER_COMM_SETUP_TUNER_CTRL_I32PLLSTEPSIZE_INDEX + TUNER_COMM_SETUP_TUNER_CTRL_I32PLLSTEPSIZE_SIZE - MDTV_HEADER_SIZE)

//----------------------------------------------------------------------------
// Function              TUNER_setupTunerControl
// Direction             Response

//return
#define TUNER_RESP_SETUP_TUNER_CTRL_RETURN_INDEX				(MDTV_HEADER_SIZE)
#define TUNER_RESP_SETUP_TUNER_CTRL_RETURN_SIZE					(4)

//all data
#define TUNER_RESP_SETUP_TUNER_CTRL_SIZE						(TUNER_RESP_SETUP_TUNER_CTRL_RETURN_INDEX + TUNER_RESP_SETUP_TUNER_CTRL_RETURN_SIZE - MDTV_HEADER_SIZE)

//----------------------------------------------------------------------------
// Function              TUNER_setupSCPConfig
// Direction             Command

//index
#define TUNER_COMM_SETUP_SCP_CONFIG_UI8INDEX_INDEX				(MDTV_HEADER_SIZE)
#define TUNER_COMM_SETUP_SCP_CONFIG_UI8INDEX_SIZE				(1)

//ADCFormat
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32ADCFORMAT_INDEX			(TUNER_COMM_SETUP_SCP_CONFIG_UI8INDEX_INDEX + TUNER_COMM_SETUP_SCP_CONFIG_UI8INDEX_SIZE)
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32ADCFORMAT_SIZE			(4)

//sampleRate
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32SAMPLERATE_INDEX		(TUNER_COMM_SETUP_SCP_CONFIG_UI32ADCFORMAT_INDEX + TUNER_COMM_SETUP_SCP_CONFIG_UI32ADCFORMAT_SIZE)
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32SAMPLERATE_SIZE			(4)

//CICFactor
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32CICFACTOR_INDEX			(TUNER_COMM_SETUP_SCP_CONFIG_UI32SAMPLERATE_INDEX + TUNER_COMM_SETUP_SCP_CONFIG_UI32SAMPLERATE_SIZE)
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32CICFACTOR_SIZE			(4)

//FIRFactor
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32FIRFACTOR_INDEX			(TUNER_COMM_SETUP_SCP_CONFIG_UI32CICFACTOR_INDEX + TUNER_COMM_SETUP_SCP_CONFIG_UI32CICFACTOR_SIZE)
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32FIRFACTOR_SIZE			(4)

//resamplerValue
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32RESAMPLERVALUE_INDEX	(TUNER_COMM_SETUP_SCP_CONFIG_UI32FIRFACTOR_INDEX + TUNER_COMM_SETUP_SCP_CONFIG_UI32FIRFACTOR_SIZE)
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32RESAMPLERVALUE_SIZE		(4)

//narrowBandFIRCoeffs[12]
#define TUNER_COMM_SETUP_SCP_CONFIG_AI16NARROWBANDFIRCOEFFS_INDEX	(TUNER_COMM_SETUP_SCP_CONFIG_UI32RESAMPLERVALUE_INDEX + TUNER_COMM_SETUP_SCP_CONFIG_UI32RESAMPLERVALUE_SIZE)
#define TUNER_COMM_SETUP_SCP_CONFIG_AI16NARROWBANDFIRCOEFFS_SIZE	(24)

//wideBandFIRCoeffs[12]
#define TUNER_COMM_SETUP_SCP_CONFIG_AI16WIDEBANDFIRCOEFFS_INDEX	(TUNER_COMM_SETUP_SCP_CONFIG_AI16NARROWBANDFIRCOEFFS_INDEX + TUNER_COMM_SETUP_SCP_CONFIG_AI16NARROWBANDFIRCOEFFS_SIZE)
#define TUNER_COMM_SETUP_SCP_CONFIG_AI16WIDEBANDFIRCOEFFS_SIZE	(24)

//rapidAGCPeriod
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32RAPIDAGCPERIOD_INDEX	(TUNER_COMM_SETUP_SCP_CONFIG_AI16WIDEBANDFIRCOEFFS_INDEX + TUNER_COMM_SETUP_SCP_CONFIG_AI16WIDEBANDFIRCOEFFS_SIZE)
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32RAPIDAGCPERIOD_SIZE		(4)

//normalAGCPeriod
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32NORMALAGCPERIOD_INDEX	(TUNER_COMM_SETUP_SCP_CONFIG_UI32RAPIDAGCPERIOD_INDEX + TUNER_COMM_SETUP_SCP_CONFIG_UI32RAPIDAGCPERIOD_SIZE)
#define TUNER_COMM_SETUP_SCP_CONFIG_UI32NORMALAGCPERIOD_SIZE	(4)

//all data
#define TUNER_COMM_SETUP_SCP_CONFIG_SIZE						(TUNER_COMM_SETUP_SCP_CONFIG_UI32NORMALAGCPERIOD_INDEX + TUNER_COMM_SETUP_SCP_CONFIG_UI32NORMALAGCPERIOD_SIZE - MDTV_HEADER_SIZE)

//----------------------------------------------------------------------------
// Function              TUNER_setupSCPConfig
// Direction             Response

//return
#define TUNER_RESP_SETUP_SCP_CONFIG_RETURN_INDEX				(MDTV_HEADER_SIZE)
#define TUNER_RESP_SETUP_SCP_CONFIG_RETURN_SIZE					(4)

//all data
#define TUNER_RESP_SETUP_SCP_CONFIG_SIZE						(TUNER_RESP_SETUP_SCP_CONFIG_RETURN_INDEX + TUNER_RESP_SETUP_SCP_CONFIG_RETURN_SIZE - MDTV_HEADER_SIZE)


#endif /* _MDTV_TUNER_H_ */
