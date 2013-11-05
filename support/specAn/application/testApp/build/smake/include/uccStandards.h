/*!
 *****************************************************************************

 @file      uccStandards.h
 @brief     Enumeration of supported UCC standards

 Copyright (c) Imagination Technologies Limited. 2010

 ****************************************************************************/


#ifndef _UCCSTANDARDS_H_
#define _UCCSTANDARDS_H_

/** Known supported (de)modulation standards.
 *
 * This type is defined for use where it is necessary to indicate to a software component
 * precisely which (de)modulation standard is being processed.
 *
 * For example, some tuners require
 * standard-specific configuration, so the standard tuner driver's configuration function accepts
 * an argument of this type. However, this kind of standard indication is not necessarily limited
 * to the tuner, so to ensure a consistent approach across all UCC software, we maintain a common
 * definition at the "top level" of the UCC Runtime.
 */

/*
 * These values are used in tuner drivers and in the common TV API so it's important they aren't changed
 * Add new definitions at the end, so as not to change
 * the existing values.
 */
typedef enum
{
    /** Standard not signalled - for use where the demodulation standard is unknown. */
    UCC_STANDARD_NOT_SIGNALLED = 0,
    /** DVB-T */
    UCC_STANDARD_DVBT,
    /** DVB-H */
    UCC_STANDARD_DVBH,
    /** ISDB-T single segment */
    UCC_STANDARD_ISDBT_1SEG,
    /** ISDB-T three segment */
    UCC_STANDARD_ISDBT_3SEG,
    /** ISDB-T thirteen segment */
    UCC_STANDARD_ISDBT_13SEG,
    /** DAB (this can include T-DMB and DMB Digital Radio) */
    UCC_STANDARD_DAB,
    /** FM (potentially with RDS) */
    UCC_STANDARD_FM,
    /** ATSC */
    UCC_STANDARD_ATSC,
    /** DVB-C */
    UCC_STANDARD_DVBC,
    /** J83B */
    UCC_STANDARD_J83B,
    /** ISDB-C */
    UCC_STANDARD_ISDBC,
    /** DVB-T2 */
    UCC_STANDARD_DVBT2,
    /** DVB-S */
    UCC_STANDARD_DVBS,
    /** DVB-S2 */
    UCC_STANDARD_DVBS2,
    /** ISDB-S */
    UCC_STANDARD_ISDBS,
    /** GB206 */
    UCC_STANDARD_GB206,
    /** Analogue TV */
    UCC_STANDARD_ATV,
    /** AM radio, analogue */
    UCC_STANDARD_AM,
    /** HD-radio, AM */
    UCC_STANDARD_HDR_AM,
    /** HD-radio, FM */
    UCC_STANDARD_HDR_FM,
    /** Digital Radio Mondiale */
    UCC_STANDARD_DRM
} UCC_STANDARD_T;

#endif /* _UCCSTANDARDS_H_ */
