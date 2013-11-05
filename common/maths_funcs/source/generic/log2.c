/*
** FILE NAME:   $RCSfile: log2.c,v $
**
** TITLE:       Log base 2
**
** PROJECT:		UCCP Common
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Arithmetric support functions.
**
**				Copyright (C) 2009, Imagination Technologies Ltd.
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

/*
** Includes
*/
/* Keep these first ... */
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include "maths_funcs.h"


#define MATHS_LOG2_INPUT_FRACT_BITS   (16)
#define MATHS_LOG2_OUTPUT_FRACT_BITS  (16)
#define MATHS_LOG2_LOOKUP_BITS        (4)
#define MATHS_LOG2_MIN_LOG            (-20<<MATHS_SHIFT_Q16)
#define MATHS_LOG2_NORM_POS           (32)
#define MATHS_LOG2_NORM_CORRECTION    (MATHS_LOG2_NORM_POS - MATHS_LOG2_INPUT_FRACT_BITS - 2)
#define MATHS_LOG2_LOOKUP_RSHIFT      (MATHS_LOG2_NORM_POS - MATHS_LOG2_LOOKUP_BITS)
#define MATHS_LOG2_LOOKUP_MASK        ((1<<MATHS_LOG2_LOOKUP_BITS)-1)
#define MATHS_LOG2_XTRUNC_BITS        (12)
#define MATHS_LOG2_XTRUNC_RSHIFT      (MATHS_LOG2_NORM_POS - MATHS_LOG2_LOOKUP_BITS - MATHS_LOG2_XTRUNC_BITS)
#define MATHS_LOG2_XTRUNC_MASK        ((1<<MATHS_LOG2_XTRUNC_BITS)-1)
#define MATHS_LOG2_MTAB_SHIFT         (2)
#define MATHS_LOG2_MX_SCALE           ((MATHS_LOG2_MTAB_SHIFT + MATHS_LOG2_XTRUNC_BITS + MATHS_LOG2_LOOKUP_BITS) - MATHS_LOG2_OUTPUT_FRACT_BITS)

// Gradient and intercept tables
const long mtab[] = {14,14,14,14,14,14,14,12,11,10,9,8,7,7,6,6};  // Q.2 number format
const long ctab[] = {1856<<4, 1855<<4, 941<<4, 3056<<4, 345<<4, 1597<<4, 2633<<4, 3512<<4, 1<<4, 690<<4, 1314<<4, 1886<<4, 2409<<4, 2865<<4, 3320<<4, 3715<<4};    /* Q.16 format */

/*
** FUNCTION:    MATHS_log2()
**
** DESCRIPTION: Convert a value from linear form Q15.16 to log2(in)
**              with the result presented in Q15.16 format
**
** PARAMETERS:  Input value in Q15.16 form (ie 16 fractonal bits)
**
** RETURNS:     long    - resultant value in dB Q15.16 format
*/
long MATHS_log2(long linear)
{
    long norm, xnorm, xtrunc, log2, index=0;

    /* bounds check */
    if (linear <= 0)
        return MATHS_LOG2_MIN_LOG;

    /* Normalise input. This gives us the integer part of log base 2 */
    norm=0;
    xnorm=linear;
    while (((xnorm<<=1) & (1<<(MATHS_LOG2_NORM_POS-1))) == 0)
        norm++;

    /* Now use LUT of gradient and intecepts to get fractional part */
    index  = (xnorm >> MATHS_LOG2_LOOKUP_RSHIFT) & MATHS_LOG2_LOOKUP_MASK;
    xtrunc = (xnorm >> MATHS_LOG2_XTRUNC_RSHIFT) & MATHS_LOG2_XTRUNC_MASK;
    log2   = (mtab[index] * xtrunc) >> MATHS_LOG2_MX_SCALE;  // Q.2 * Q.16 -> Q.18 hence >> 2 to match Q.16 ctab format
    log2  += ctab[index];

    /* Form log2(linear) Q.16 format */
    log2   = ((MATHS_LOG2_NORM_CORRECTION - norm) << MATHS_LOG2_OUTPUT_FRACT_BITS) + log2;

    if (log2 < MATHS_LOG2_MIN_LOG)
        log2 = MATHS_LOG2_MIN_LOG;

    return log2;
}




