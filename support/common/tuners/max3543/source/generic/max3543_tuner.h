/*!
******************************************************************************

 @file max3543_tuner.h

 @brief Maxim 3543 tuner driver

 @Author Imagination Technologies

	<b>Copyright (C) Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for the Maxim MAX3543 tuner driver.

******************************************************************************/
/*! \mainpage Maxim Max3543 tuner driver
*******************************************************************************
 \section intro Introduction

 This is a driver for the Maxim MAX3543 tuner driver.

 <b>Feedback</b>

 If you have any comments regarding this document, please contact your
 Imagination Technologies representative.
 Please provide the document title and revision with a description of the problem
 or suggestion for improvement.

*******************************************************************************/

#ifndef MAX3543_TUNER_H
/* @cond DONT_DOXYGEN */
#define MAX3543_TUNER_H

#include "PHY_tuner.h"
/* @endcond */


/*!
*******************************************************************************
 Maxim MAX3543 tuner driver configuration.
 The various aspect of the driver that can be configured at run time.
*******************************************************************************/
typedef struct
{
	/*! Serial port number.
	    With SCBM port numbers 1 and 2 correspond to SCBM_PORT_1 and SCBM_PORT_2.
	*/
	int portNumber;
}MAX3543_CONFIG_T;

/*! Exported tuner control structure for Maxim Max3543 tuner driver */
extern TUNER_CONTROL_T MAX3543Tuner;

/*!
*******************************************************************************

 @Function              @MAX3543Tuner_configure

 <b>Description:</b>\n
 This function configures various aspects of the Max3543 tuner driver, allowing it
 to be used in various systems.
 This must be called prior to the tuner control structure being registered, so
 that we can ensure that the driver is configured prior to it being used.

 \param[in]     config		Configuration of the tuner driver.

 \return					void

*******************************************************************************/
void MAX3543Tuner_configure(MAX3543_CONFIG_T *config);

#endif
