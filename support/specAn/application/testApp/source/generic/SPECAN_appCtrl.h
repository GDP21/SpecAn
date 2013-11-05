/*!****************************************************************************
 @File          SPECAN_appCtrl.h

 @Title         Application support code

 @Date          3 Dec 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2012

******************************************************************************/

#ifndef _SPECAN_APP_CTRL_H_
#define _SPECAN_APP_CTRL_H_

#include "uccframework.h"

/**
 * Structure to identify a single event flag for use by SA_APP_mapEF
 */
typedef struct
{
    KRN_FLAGCLUSTER_T *flagCluster;
    unsigned flagMask;
} SPECAN_APP_EVENT_FLAG_T;


/**
 * Tune - set  RF frequency and bandwidth
 *
 * @param instance:	Pointer to the TV core instance
 * @param frequency: RF frequency (Hz)
 * @param bandwidth: RF bandwidth (Hz)
 */
void SA_APP_tune(UFW_COREINSTANCE_T *instance, unsigned frequency,
			unsigned bandwidth);


/**
 * Switch the system from INITIALISED into the DETECT state (from which it will automatically move to
 * COMPLETED once scan is complete).
 *
 * @param[in] instance Pointer to the TV core instance
 * @param[in] stateFlag Pointer to a ::TVXAPP_EVENT_FLAG_T which is assumed to be set
 *                            whenever the TV core changes state. This must have been previously set up using
 *                            ::TVXAPP_mapEF().
 * @param[in] timeout Timeout value
 * @returns true: State changed OK, false: state not changed. This can happen if the TV is not in a suitable
 * initial state or if no state changes occur within the \c timeout period.
 */
bool SA_APP_startScan(UFW_COREINSTANCE_T *instance,
		SPECAN_APP_EVENT_FLAG_T *stateFlag, int timeout);


/**
 * Associate an event flag with a core interface register
 *
 * @param instance Pointer to the TV core instance
 * @param regId Register Id
 * @param eventFlag Pointer to structure defining the event flag to set when the register updates.
 * 			NULL to clear the event flag association.
 */
void SA_APP_mapEF(UFW_COREINSTANCE_T *instance, unsigned regId,
			SPECAN_APP_EVENT_FLAG_T *eventFlag);

#endif /* _SPECAN_APP_CTRL_H_ */
