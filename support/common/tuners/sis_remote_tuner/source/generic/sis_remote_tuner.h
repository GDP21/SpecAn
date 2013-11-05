/*!
******************************************************************************

 @file example_tuner.h

 @brief SIS remote tuner driver

 @Author Ensigma

	<b>Copyright (C) 2008-2009, Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for example tuner driver

******************************************************************************/
/*! \mainpage SIS remote tuner driver
*******************************************************************************
 \section intro Introduction

 The tuner is actually controlled by the SIS SOC firmware and outputs at 36MHz IF

 <b>Feedback</b>

 If you have any comments regarding this document, please contact your
 Imagination Technologies representative.
 Please provide the document title and revision with a description of the problem
 or suggestion for improvement.

*******************************************************************************/

#ifndef SIS_REMOTE_TUNER_H
/* @cond DONT_DOXYGEN */
#define SIS_REMOTE_TUNER_H
#include "PHY_tuner.h"
/* @endcond */

extern TUNER_CONTROL_T sisRemoteTuner;
extern TUNER_CONTROL_T sisRemoteTunerNearZeroIF;

#endif
