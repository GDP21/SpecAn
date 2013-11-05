/*!
******************************************************************************

 @file example_scp_config_tdmb.h

 @brief Example SCP configuration

 @Author Ensigma

	<b>Copyright (C) 2008-2009, Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for example T-DMB SCP configuration

******************************************************************************/
/*! \mainpage Example T-DMB SCP configuration
*******************************************************************************
 \section intro Introduction

 This is an example of how to produce a configuration for the SCP.
 The sample rate and decimation factors are all consistent and provide a very
 simple example of how these should be used.
 The filter co-efficients provided are those we recommend to use with T-DMB.

 <b>Feedback</b>

 If you have any comments regarding this document, please contact your
 Imagination Technologies representative.
 Please provide the document title and revision with a description of the problem
 or suggestion for improvement.

*******************************************************************************/
#ifndef EXAMPLE_SCP_CONFIG_H
/* @cond DONT_DOXYGEN */
#define EXAMPLE_SCP_CONFIG_H
#include "PHY_scpConfig.h"
/* @endcond */

/*! Example of a SCP configuration for T-DMB that can be registered with the Tuner API. */
extern PHY_SCP_CONFIG_T exampleSCPConfig_TDMB;

#endif
