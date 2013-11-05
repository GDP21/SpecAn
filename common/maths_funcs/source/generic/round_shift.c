/*
** FILE NAME:   $RCSfile: round_shift.c,v $
**
** TITLE:       Shift down with rounding
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
** FUNCTION:    MATHS_roundAndShift()
**
** DESCRIPTION: Scale down a number using a right shift applying rounding in the process.
**
** PARAMETERS:  value           - The value to be shifted down.
**              shift           - The denominator as a right shift
**
** RETURNS:     round(value * 2^-shift)
*/
long MATHS_roundAndShift(long value, long shift)
{
    if (shift==0)
        return value;
    return ((value + (1<<(shift-1))) >> shift);
}

