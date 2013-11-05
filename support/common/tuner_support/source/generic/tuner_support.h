/*!
******************************************************************************

 @file tuner_support.h

 @brief Tuner support functions

 @Author Ensigma

	<b>Copyright (C) 2008-2009, Imagination Technologies Ltd.</b>\n

	This software is supplied under a licence agreement.
	It must not be copied or distributed except under the
	terms of that agreement.

 \n<b>Description:</b>\n
	Interface for tuner support functions

******************************************************************************/

#ifndef TUNER_SUPPORT_H
/* @cond DONT_DOXYGEN */
#define TUNER_SUPPORT_H
/* @endcond */

#include "PHY_tuner.h"

/*! Enumerate the tuner API functions which we can replace */
typedef enum {
	TUNER_INITIALISE = 0,
	TUNER_CONFIGURE,
	TUNER_TUNE,
	TUNER_READ_RF_POWER,
	TUNER_POWER_UP,
	TUNER_POWER_DOWN,
	TUNER_POWER_SAVE,
	TUNER_SET_IF_AGC_TIME_CONSTANT,
	TUNER_SET_AGC,
	TUNER_INIT_AGC
} TUNER_FUNC_T;

/*! Union of the tuner API functions which we can replace */
typedef union {
    PHY_TUNER_RETURN_T (*initialise)(TUNER_COMPLETION_FUNC_T);
    PHY_TUNER_RETURN_T (*configure)(PHY_TUNER_STANDARD_T ,long, TUNER_COMPLETION_FUNC_T);
    PHY_TUNER_RETURN_T (*tune)(long, TUNER_COMPLETION_FUNC_T);
    long               (*readRFPower)(TUNER_COMPLETION_FUNC_T);
    PHY_TUNER_RETURN_T (*powerUp)(unsigned long, TUNER_COMPLETION_FUNC_T);
    PHY_TUNER_RETURN_T (*powerDown)(unsigned long, TUNER_COMPLETION_FUNC_T);
    PHY_TUNER_RETURN_T (*powerSave)(PHY_RF_PWRSAV_T, unsigned long, TUNER_COMPLETION_FUNC_T);
    PHY_TUNER_RETURN_T (*setIFAGCTimeConstant)(long, TUNER_COMPLETION_FUNC_T);
    PHY_TUNER_RETURN_T (*setAGC)(TUNER_AGCISR_HELPER_T *, TUNER_COMPLETION_FUNC_T);
    PHY_TUNER_RETURN_T (*initAGC)(unsigned long, TUNER_COMPLETION_FUNC_T);
} TUNER_ROUTINE_T;

/*! Function type for optional level translator function called by TUNER_getRmsLevel() */
typedef float (*TUNER_LEVEL_TRANS_ROUTINE_T)(float);


/*!
*******************************************************************************

 @Function              @TUNER_installFilter

 <b>Description:</b>\n
 This function replaces the specified function in the tuner driver by the
 given function. Hence it allows tuner driver functions to be overridden.
 Often the replacement function will call the displaced function setting up a
 daisy-chain of function calls.

 \param[in]     tuner	Tuner driver structure of tuner driver to install function.
 \param[in]     func	Which function to install.
 \param[in]     routine	The function to install.

 \return				The function in the tuner driver that was replaced
 						when we installed the given function.

*******************************************************************************/
TUNER_ROUTINE_T TUNER_installFilter(TUNER_CONTROL_T *tuner, TUNER_FUNC_T func,
    TUNER_ROUTINE_T routine);

/*!
*******************************************************************************

 @Function              @TUNER_removeFilter

 <b>Description:</b>\n
 This function replaces the specified function in the tuner driver by the
 given function. Is intended to restore a function returned by a
 previous call to TUNER_installFilter().

 \param[in]     tuner	Tuner driver structure of tuner driver from which to
 						remove function.
 \param[in]     func	Which function to remove.
 \param[in]     routine	The function to remove.

*******************************************************************************/
void TUNER_removeFilter(TUNER_CONTROL_T *tuner, TUNER_FUNC_T func,
    TUNER_ROUTINE_T routine);

/*!
*******************************************************************************

 @Function              @TUNER_installLevelTrans

 <b>Description:</b>\n
 This function installs the optional level translator function called by
 TUNER_getRmsLevel().

 \param[in]     routine	Level translator function.

 \return				Previously installed level translator function.
 						Will be NULL if no function installed.

*******************************************************************************/
TUNER_LEVEL_TRANS_ROUTINE_T TUNER_installLevelTrans(
    TUNER_LEVEL_TRANS_ROUTINE_T routine);

/*!
*******************************************************************************

 @Function              @TUNER_removeLevelTrans

 <b>Description:</b>\n
 This function installs the optional level translator function called by
 TUNER_getRmsLevel(). Is intended to restore a function returned by a
 previous call to TUNER_installLevelTrans().

 \param[in]     routine		Level translator function to replace install one

*******************************************************************************/
void TUNER_removeLevelTrans(TUNER_LEVEL_TRANS_ROUTINE_T routine);

/*!
*******************************************************************************

 @Function              @TUNER_getRmsLevel

 <b>Description:</b>\n
 This function calculates the RMS signal level (in dBm) from the AGC status.
 Optionally the installed level translator function is called.

 \param[in]     pControl	AGC status info structure.

 \return					RMS signal level estimate (dBm).

*******************************************************************************/
float TUNER_getRmsLevel(TUNER_AGCISR_HELPER_T *pControl);

#endif
