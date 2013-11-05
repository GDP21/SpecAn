/*!
******************************************************************************

 @file stv6110_tuner.h

 @brief Satellite tuner driver

 @Author Imagination Technologies

	<b>Copyright (C) Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for the satellite tuner driver based upon the STV6110.

******************************************************************************/
/*! \mainpage STV6110 STV tuner driver
*******************************************************************************
 \section intro Introduction

 This is a satellite tuner driver based upon the STV6110.

 <b>Feedback</b>

 If you have any comments regarding this document, please contact your
 Imagination Technologies representative.
 Please provide the document title and revision with a description of the problem
 or suggestion for improvement.

*******************************************************************************/

#ifndef STV6110_TUNER_H
/* @cond DONT_DOXYGEN */
#define STV6110_TUNER_H

#include "uccrt.h"
/* @endcond */


/*!
*******************************************************************************
 STV6110 satellite tuner driver configuration.
 The various aspect of the driver that can be configured at run time.
*******************************************************************************/
typedef struct
{
	/*! Serial port number.
	    The exact meaning of this is dependant upon the SOC and its I2C driver.
	*/
	int portNumber;
	/*! I2C address of tuner
	*/
	int address;
}STV6110_CONFIG_T;

/*! Size in bytes of the workspace that needs to be allocated for this tuner driver. */
#define STV6110_TUNER_DRIVER_WORKSPACE_SIZE	4500

/*! Exported tuner control structure for STV tuner driver */
extern TDEV_CONFIG_T STV6110Tuner;

#endif
