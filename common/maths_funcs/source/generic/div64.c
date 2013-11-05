/*
** FILE NAME:   $RCSfile: div64.c,v $
**
** TITLE:       High precision 64 bit division
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


/*
** FUNCTION:    MATHS_highPrecisionDiv64
**
** DESCRIPTION: High precision division of two signed 64-bit numbers implemented by normalisation of the numerator
**              to maintain optimum precision.
**
** PARAMETERS:  long long num, den - numerator and denominator
**
** RETURNS:     long long num/den
*/
long long MATHS_highPrecisionDiv64(long long num, long long den)
{
    int scale;
    long long intermediate, result, round=0;

    /* Trap zero numerator */
    if (num==0)
    {
        return 0LL;
    }

    /* Scale until the sign changes, this indicates overflow */
    if (num>0)
    {
        for(scale=0; (num<<scale) > 0; scale++)
            ;
    }
    else
    {
        for(scale=0; (num<<scale) < 0; scale++)
            ;
    }

    if (scale > 0)
        scale--;

    intermediate = (num<<scale) / den;

    if (scale > 0)
        round = 1<<(scale-1);

    result = (intermediate + round) >> scale;

    return result;
}

