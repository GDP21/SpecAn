/*
** FILE NAME:   $RCSfile: divq16.c,v $
**
** TITLE:       Q15.16 division
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
** FUNCTION:    MATHS_divQ16
**
** DESCRIPTION: Fixed-point divide of two values.
**              input and result formats are Q15.16
*/
long MATHS_divQ16(long numerator, long denominator)
{
    long neg=0, normShift=0, intResult;

    if ((denominator == 0) || (numerator == 0))
    {
        if (numerator < 0)
            return (0x80000000);
        else if (numerator > 0)
            return (0x7fffffff);
        else
            return 0;
    }

    /* Work on positive values only */
    if (numerator < 0)
    {
        if (numerator == 0x8000000)
            numerator++;
        numerator = 0-numerator;
        neg = 1;
    }

    /* Normalise the numerator */
    while ((numerator<<normShift)<0x40000000)
        normShift++;
    numerator <<= normShift;

    /* Straight integer divide */
    intResult = numerator / denominator;

    /* Rescale to Q16 result */
    normShift = MATHS_SHIFT_Q16 - normShift;

    if (normShift < 0)
        return (intResult >> (-normShift));

    return (intResult << normShift);
}
