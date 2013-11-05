/*!
******************************************************************************

 @file example_soc_config.h

 @brief Example SOC config

 @Author Ensigma

	<b>Copyright (C) 2009, Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for example SOC configuration driver

******************************************************************************/
/*! \mainpage Example SOC configuration driver
*******************************************************************************
 \section intro Introduction

 This is an example of how to build in a SOC configuration driver into a
 demodulation standard.
 The driver provided is a completely empty framework for users of the IP
 to fill in the details for the SOC they wish to support.
 This is be included with the example applications and can be selected as an
 option to build into them (see individual standards documentation to see how to do this).

 <b>Feedback</b>

 If you have any comments regarding this document, please contact your
 Imagination Technologies representative.
 Please provide the document title and revision with a description of the problem
 or suggestion for improvement.

*******************************************************************************/

#ifndef EXAMPLE_SOC_CONFIG_H
#define EXAMPLE_SOC_CONFIG_H

/*!
*******************************************************************************

 @Function              @ExampleSOCConfig

 <b>Description:</b>\n
 This function configures SOC specific aspects of the system at startup.

*******************************************************************************/
void ExampleSOCConfig(void);

#endif
