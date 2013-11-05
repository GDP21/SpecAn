/*!****************************************************************************
 @File          SPECAN_timestamp.h

 @Title         Spectrum analyser timestamping

 @Date          28 January 2013

 @Copyright     Copyright (C) Imagination Technologies Limited 2013

 ******************************************************************************/
#ifndef _SPECAN_TIMESTAMP_H_
#define _SPECAN_TIMESTAMP_H_

#define MAX_STATES	(10)

/* Context structure for timestamp measurement */
typedef struct {
    unsigned  start;
    unsigned  end;
    unsigned  stamps[MAX_STATES];
    unsigned  stateCnt[MAX_STATES];
    unsigned  lastValidTime;
    unsigned  total;
} TIME_STAMP_T;

/*----------------------------------------------------------------------*/
void initTimeStamp(TIME_STAMP_T *pTimeStamp);

/*----------------------------------------------------------------------*/
/* Log times after recordAcquisitionStart has been called.
 * Return 0 for success,
 * -1 for invalid state ID
 * -2 if recordAcquisitionStart not yet called .
 */
int recordTimeStamp(TIME_STAMP_T *pTimeStamp, unsigned state);

/*----------------------------------------------------------------------*/
void recordAcquisitionStart(TIME_STAMP_T *pTimeStamp);

/*----------------------------------------------------------------------*/
void recordAcquisitionEnd(TIME_STAMP_T *pTimeStamp);


#endif /* _SPECAN_TIMESTAMP_H_ */
