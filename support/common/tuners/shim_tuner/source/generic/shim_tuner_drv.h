/*!
******************************************************************************

 @file shim_tuner_drv.h

 @brief Shim tuner driver

 @Author Ensigma

	<b>Copyright (C) 2008-2009, Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for shim tuner driver

******************************************************************************/
/*! \mainpage shim tuner driver
*******************************************************************************
 \section intro Introduction

 This is a driver to interface between 'old' and 'new' style tuner API's.

 <b>Feedback</b>

 If you have any comments regarding this document, please contact your
 Imagination Technologies representative.
 Please provide the document title and revision with a description of the problem
 or suggestion for improvement.

*******************************************************************************/

#ifndef SHIM_TUNER_H
/* @cond DONT_DOXYGEN */
#define SHIM_TUNER_H
#include "PHY_tuner.h"
#include <uccrt.h>
/* @endcond */

/*! Example of a tuner control structure that can be registered with the Tuner API. */
extern TUNER_CONTROL_T shimTuner;

void shim_tunerDrv_configure(TDEV_CONFIG_T *tdev_driver, void *extension);

#endif
