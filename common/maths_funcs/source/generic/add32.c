/*
** FILE NAME:   $RCSfile: add32.c,v $
**
** TITLE:       32 bit saturating add
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
#include <limits.h>

#include "maths_funcs.h"

/*
** FUNCTION:    MATHS_saturatingAdd32()
**
** DESCRIPTION: Add two signed 32-bit integers and protect against saturation
**
** PARAMETERS:  a, b,   Integers to be added
**
** RETURNS:     long    saturated result
*/
long MATHS_saturatingAdd32(long a, long b)
{
    long result = a + b;

    if ((b>0) && (result < a))
        return LONG_MAX;

    if ((b<0) && (result > a))
        return LONG_MIN;

    return result;
}
