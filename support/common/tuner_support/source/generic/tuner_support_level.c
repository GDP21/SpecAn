/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuner_support/source/generic/tuner_support_level.c,v $
**
** TITLE:       Tuner support functions
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Implementation of tuner support level estimation functions
**
** NOTICE:      Copyright (C) 2008-2009, Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
*/

#ifdef METAG
/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif
#include <stddef.h>
#include <math.h>
#include "PHY_tuner.h"
#include "tuner_support.h"

#define AGC_THRESH_BITS 11
#define AGC_THRESH_FS (1 << AGC_THRESH_BITS)

/* Convert from threshold to RMS. This comes from:
 * erf(0.6745/sqrt(2)) = 0.5
 */
#define THRESH_TO_RMS (0.6745f)

/* 10logR where R is 50 */
#define TENLOGR 16.989700043360188047862611052755f

extern TUNER_LEVEL_TRANS_ROUTINE_T levelTransRoutine;

float TUNER_getRmsLevel(TUNER_AGCISR_HELPER_T *pControl)
{
    float rms;

    /*
     * Calculate RMS level
     */

    /* Read AGC threshold */
    rms = pControl->pSCPcontrol->AGCthresh1/(float)AGC_THRESH_FS;

    /* Convert threshold into RMS */
    rms *= THRESH_TO_RMS;

    /* Convert into dBm */
    rms = 30.0f + 20.0f*log10f(rms) - TENLOGR;

    /* Call additional level translator (if installed) */
    if (levelTransRoutine != NULL)
    {
        rms = (*levelTransRoutine)(rms);
    }

    return rms;
}
