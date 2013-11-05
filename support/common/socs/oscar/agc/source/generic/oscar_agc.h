/*!
******************************************************************************

 @file oscar_agc.h

 @brief Oscar AGC

 @Author Ensigma

	<b>Copyright (C) 2008-2009, Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for Oscar AGC code

******************************************************************************/

#ifndef OSCAR_AGC_H
/* @cond DONT_DOXYGEN */
#define OSCAR_AGC_H

#include "PHY_tuner.h"
/* @endcond */

/*!
*******************************************************************************

 @Function              @OSCAR_AGC_install

 <b>Description:</b>\n
 This function installs the AGC code into the tuner driver given.

 \param[in]     tuner	Tuner driver structure, into which the AGC code is installed.

*******************************************************************************/
void OSCAR_AGC_install(TUNER_CONTROL_T *tuner);
/*!
*******************************************************************************

 @Function              @OSCAR_AGC_remove

 <b>Description:</b>\n
 This function removes the AGC code from the tuner driver it was previously
 installed in.

*******************************************************************************/
void OSCAR_AGC_remove(void);

#endif
