/*
** FILE NAME:   $RCSfile: agc_constants.h,v $
**
** TITLE:       AGC Constants
**
** PROJECT:     UCCP Common
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Constants defining the characteristics of the AGC loops etc.
**
**              Copyright (C) Imagination Technologies Ltd.
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/
#ifdef __DVBC__
#define CABLE_PARAMS
#endif

#ifdef __J83B__
#define CABLE_PARAMS
#endif

#ifdef __ATSC__
#define ATSC_PARMAS
#endif

#ifndef AGC_CONSTANTS_H
#define AGC_CONSTANTS_H

#include "uccrt.h"

/* Gain calc constants */
#define AGC_THRESHOLD_VALUE         (545)     /* 11.5dB backoff to Full scale magnitude 12-bit input */
#define AGC_MAX_TGT                 (950)
#define AGC_MIN_TGT                 (250)
#define AGC_MAX_ERR                 (2)
#define AGC_TARGET_ERR_SCALE        (5)
#define AGC_INTSCALE                (8)
#define AGC_TGT_CLIPRATE_PPM        (200)    /* target clip rate [ppm] */

/* IF GAN can only be between these values */
#define ACG_IF_MIN_GAIN             (0x0000)
#define ACG_IF_MAX_GAIN             (TDEV_MAX_IF_GAIN)
/* Start off gain at 2/3 point, we reduce the gain twice as fast as we increase it. Hence in an attempt to keep the max convergence time +/- the same, put the starting point 2/3 full scale */
#define INITIAL_GAIN_ACCUM          ((2 * TDEV_MAX_IF_GAIN) / 3)

#define AGC_IQ_SCALE_SHIFT          (3)


/* ATSC Specific Parameters */
#ifdef ATSC_PARMAS
#ifdef __UCCP320_6__
#define AGC_CLIP_LEVEL              (1560)    /*  2.36dB backoff to Full scale magnitude 12-bit input */
#endif
#endif

/* Cable Specific Parameters */
#ifdef CABLE_PARAMS

#ifdef RF_SI2153
#define AGC_CLIP_LEVEL              (1705)  /*  1.6dB backoff to Full scale magnitude 12-bit input for SiLab2153 Tuner */
#else
#define AGC_CLIP_LEVEL              (1500)  /*  2.7dB backoff to Full scale magnitude 12-bit input for others */
#endif

#ifdef __UCCP320_2__
#define AGC_SCALE_SHIFT_NORMAL_UP   (6)
#define AGC_SCALE_SHIFT_NORMAL_DN   (5)
#else //__UCCP320_2__ not defined
#define AGC_SCALE_SHIFT_NORMAL_UP   (5)
#define AGC_SCALE_SHIFT_NORMAL_DN   (4)
#endif//__UCCP320_2__

#endif//CABLE_PARAMS


/* GB206 Specific Parameters */
#ifdef __GB20600__
#define AGC_SCALE_SHIFT_RAPID_UP    (3)
#define AGC_SCALE_SHIFT_RAPID_DN    (2)
#endif //GB206


/* Default values  */
#ifndef AGC_SCALE_SHIFT_NORMAL_UP
#define AGC_SCALE_SHIFT_NORMAL_UP   (5)
#endif
#ifndef AGC_SCALE_SHIFT_NORMAL_DN
#define AGC_SCALE_SHIFT_NORMAL_DN   (4)
#endif
#ifndef AGC_SCALE_SHIFT_RAPID_UP
#define AGC_SCALE_SHIFT_RAPID_UP    (2)
#endif
#ifndef AGC_SCALE_SHIFT_RAPID_DN
#define AGC_SCALE_SHIFT_RAPID_DN    (1)
#endif
#ifndef AGC_CLIP_LEVEL
#define AGC_CLIP_LEVEL              (1723)    /*  1.5dB backoff to Full scale magnitude 12-bit input */
#endif

#ifndef AGC_SCALE_MUL
#define AGC_SCALE_MUL               (1<<8)  /* Unity in Q.8 */
#endif

#define INITIAL_TARGET_ACCUM        (AGC_THRESHOLD_VALUE << AGC_INTSCALE)


/* IQ Offset constants */
#define AGC_EARLY_GAIN_SHIFT        (8)
#define IQ_CORR_SCALE_SHIFT         (10)
#define IQGAIN_ERR_SHIFT            (8)

/* IQ correction is a 11 bit value */
#define MAX_IQ_CORRECTION           ((1<<10) - 1)
#define MIN_IQ_CORRECTION           (-(1<<10))

/* Early gain also 11 bits */
#define MAX_EARLY_GAIN              ((1<<10) - 1)
#define MIN_EARLY_GAIN              (-(1<<10))


/* DC offset constants */
#define SCP_DC_OFFSET_MONITOR_BITS          (24)
#define SCP_DC_OFFSET_CTRL_FRACT_BITS       (12)            /* Number of fractional bits available to SCP internal DC canceller */
#define SCP_DC_OFFSET_CTRL_MIN_VALUE        (-(1<<23))      /* Minimum value to SCP internal DC canceller */
#define SCP_DC_OFFSET_CTRL_MAX_VALUE        ((1<<23)-1)     /* Maximum value to SCP internal DC canceller */
#define SCP_DC_OFFSET_DEFAULT_GAIN_SHIFT    (3)
#define SCP_DC_OFFSET_DEFAULT_GAIN_MULT     ((long)(1.0*AGC_SCALE_MUL))
#define SCP_DC_OFFSET_DEFAULT_DEADBAND      (1<<SCP_DC_OFFSET_CTRL_FRACT_BITS)
#define SCP_DC_OFFSET_PRE_INTEGRATIONS_LOG2 (3)
#define SCP_DC_OFFSET_PRE_INTEGRATIONS      (1<<SCP_DC_OFFSET_PRE_INTEGRATIONS_LOG2)

#endif // AGC_CONSTANTS_H
