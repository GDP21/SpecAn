/*!****************************************************************************
 @File          SPECAN_timestamp.c

 @Title         Spectrum analyser timestamping

 @Date          28 January 2013

 @Copyright     Copyright (C) Imagination Technologies Limited 2013

 ******************************************************************************/
#ifdef __META_FAMILY__
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include <assert.h>
#include "SPECAN_timestamp.h"


/*----------------------------------------------------------------------*/

void initTimeStamp(TIME_STAMP_T *pTimeStamp)
{
    unsigned i;

    pTimeStamp->start = 0;
    pTimeStamp->end   = 0;
    pTimeStamp->lastValidTime = 0;

    for (i=0; i<MAX_STATES; i++)
    {
        pTimeStamp->stamps[i] = 0;
        pTimeStamp->stateCnt[i] = 0;
    }
}

/*----------------------------------------------------------------------*/
/* Log times after recordAcquisitionStart has been called.
 * Return 0 for success,
 * -1 for invalid state ID
 * -2 if recordAcquisitionStart not yet called .
 * */
int recordTimeStamp(TIME_STAMP_T *pTimeStamp, unsigned state)
{
    unsigned  currentTime = TBI_GETREG(TXTIMER);

    if (pTimeStamp->start == 0)
        return -2;

    if (state >= MAX_STATES)
    	return -1;

    pTimeStamp->stamps[state] += currentTime - pTimeStamp->lastValidTime;

    pTimeStamp->lastValidTime = currentTime;
    pTimeStamp->stateCnt[state]++;

    return 0;
}

/*----------------------------------------------------------------------*/

void recordAcquisitionStart(TIME_STAMP_T *pTimeStamp)
{
	initTimeStamp(pTimeStamp);

	pTimeStamp->start = TBI_GETREG(TXTIMER);
	pTimeStamp->lastValidTime = pTimeStamp->start;
}



/*----------------------------------------------------------------------*/

void recordAcquisitionEnd(TIME_STAMP_T *pTimeStamp)
{
	pTimeStamp->end = TBI_GETREG(TXTIMER);

	/* This subtraction should wrap correctly if the timestamp has wrapped.  Note that
	the system timer should be set up for a 1MHz clock rate so our time stamps are in us.
	Divide by 1000 for a value in ms. */
	pTimeStamp->total = (pTimeStamp->end - pTimeStamp->start);
}
