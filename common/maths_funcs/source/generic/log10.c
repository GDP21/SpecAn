/*
** FILE NAME:   $RCSfile: log10.c,v $
**
** TITLE:       Log base 10
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

#define MATHS_LOG10_LOG2_SCALE_Q8     (77)    /* log10(2) */

/*
** FUNCTION:    MATHS_log10()
**
** DESCRIPTION: Convert a value from linear form Q15.16 to log10(in)
**              with the result presented in Q15.16 format
**
** PARAMETERS:  Input value in Q15.16 form (ie 16 fractonal bits)
**
** RETURNS:     long    - resultant value in dB Q15.16 format
*/
long MATHS_log10(long linear)
{
    long log2, log10;

    log2 = MATHS_log2(linear);
    log10 = (log2*MATHS_LOG10_LOG2_SCALE_Q8)>>MATHS_SHIFT_Q8;

    return log10;
}
