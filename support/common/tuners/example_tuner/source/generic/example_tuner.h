/*!
******************************************************************************

 @file example_tuner.h

 @brief Example tuner driver

 @Author Ensigma

	<b>Copyright (C) Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for example tuner driver

******************************************************************************/
/*! \mainpage Example tuner driver
*******************************************************************************
 \section intro Introduction

 This empty framework is provided as an example of how to build in a tuner driver
 into a demodulation standard using the common tuner API.

 The driver provided is a completely empty framework for users of the IP to fill
 in the details for the tuner they wish to support.
 This is be included with the example applications and can be selected as an option to
 build into them (see individual standards documentation to see how to do this).

 Note this is intended to be separate from the dummy tuner drivers supplied to work with digital playout.

 <b>Feedback</b>

 If you have any comments regarding this document, please contact your
 Imagination Technologies representative.
 Please provide the document title and revision with a description of the problem
 or suggestion for improvement.

*******************************************************************************/

#ifndef EXAMPLE_TUNER_H
/* @cond DONT_DOXYGEN */
#define EXAMPLE_TUNER_H
#include "uccrt.h"
/* @endcond */

#define EXAMPLE_TUNER_WORKSPACE_SIZE	0

/*! Example of a tuner control structure that can be registered with the Tuner API. */
extern TDEV_CONFIG_T exampleTuner;
extern TDEV_CONFIG_T exampleTunerFM;

#endif
