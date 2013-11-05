/*!
******************************************************************************

 @file mxl_dtv_tuner.h

 @brief MaxLinear DTV tuner driver

 @Author Imagination Technologies

	<b>Copyright (C) Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for the MaxLinear DTV tuner driver.

******************************************************************************/
/*! \mainpage MaxLinear DTV tuner driver
*******************************************************************************
 \section intro Introduction

 This is a driver for the MaxLinear DTV tuner driver.

 <b>Feedback</b>

 If you have any comments regarding this document, please contact your
 Imagination Technologies representative.
 Please provide the document title and revision with a description of the problem
 or suggestion for improvement.

*******************************************************************************/

#ifndef MXL_DTV_TUNER_H
/* @cond DONT_DOXYGEN */
#define MXL_DTV_TUNER_H

#include "uccrt.h"
/* @endcond */


/*!
*******************************************************************************
 MaxLinear tuner driver configuration.
 The various aspect of the driver that can be configured at run time.
*******************************************************************************/
typedef struct
{
	/*! Serial port number.
	    The exact meaning of this is dependant upon the SOC and its I2C driver.
	*/
	int portNumber;
}MXL_DTV_CONFIG_T;

/*! Size in bytes of the workspace that needs to be allocated for this tuner driver. */
#define MXL_DTV_TUNER_DRIVER_WORKSPACE_SIZE	4500

/*! Exported tuner control structure for MaxLinear DTV tuner driver */
extern TDEV_CONFIG_T MXL_DTVTuner;

#endif
