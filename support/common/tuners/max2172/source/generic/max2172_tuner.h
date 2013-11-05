/*!
******************************************************************************

 @file max2172_tuner.h

 @brief Maxim 2172 tuner driver

 @Author Imagination Technologies

	<b>Copyright (C) Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for the Maxim MAX2172 tuner driver.

******************************************************************************/
/*! \mainpage Maxim Max2172 tuner driver
*******************************************************************************
 \section intro Introduction

 This is a driver for the Maxim MAX2172 tuner driver.

 <b>Feedback</b>

 If you have any comments regarding this document, please contact your
 Imagination Technologies representative.
 Please provide the document title and revision with a description of the problem
 or suggestion for improvement.

*******************************************************************************/

#ifndef MAX2172_TUNER_H
/* @cond DONT_DOXYGEN */
#define MAX2172_TUNER_H

#include "uccrt.h"
/* @endcond */

/*!
*******************************************************************************
 Flag to differentiate between high and low side conversion.
*******************************************************************************/
typedef enum {
    /*! Select high side conversion */
    HIGH_SIDE_CONVERT,
    /*! Select low side conversion */
    LOW_SIDE_CONVERT
}
HI_LO_CONVERT_T;

/*!
*******************************************************************************
 Maxim MAX2172 tuner driver configuration.
 The various aspect of the driver that can be configured at run time.
*******************************************************************************/
typedef struct
{
	/*! Serial port number.
	    With SCBM port numbers 1 and 2 correspond to SCBM_PORT_1 and SCBM_PORT_2.
	*/
	int portNumber;
}MAX2172_CONFIG_T;

/*! Exported tuner control structure for Maxim Max2172 tuner driver */
extern TDEV16_CONFIG_T MAX2172Tuner;

/*!
*******************************************************************************

 @Function              @MAX2172Tuner_configure

 <b>Description:</b>\n
 This function configures various aspects of the Max2172 tuner driver, allowing it
 to be used in various systems.
 This must be called prior to the tuner control structure being registered, so
 that we can ensure that the driver is configured prior to it being used.

 \param[in]     config			Configuration of the tuner driver.

 \return						void

*******************************************************************************/
void MAX2172Tuner_configure(MAX2172_CONFIG_T *config);


/*!
*******************************************************************************

 @Function              @MAX2172Tuner_reconfigure

 <b>Description:</b>\n
 This function reconfigures the Max2172 tuner driver.

 This must be called prior to the 'tune' function being called.
 This is to ensure that the driver is configured correctly prior to it
 attempting to tune.

 \param[in] reqHiLoConvertFlag	Flag to control high or low side conversion.

 \return						void

*******************************************************************************/
void MAX2172Tuner_reconfigure(HI_LO_CONVERT_T reqHiLoConvertFlag);

#endif
