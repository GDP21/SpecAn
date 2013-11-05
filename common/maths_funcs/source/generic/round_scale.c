/*
** FILE NAME:   $RCSfile: round_scale.c,v $
**
** TITLE:       Scale down with rounding
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
** FUNCTION:    MATHS_roundAndScale()
**
** DESCRIPTION: Scale down a number applying rounding in the process.
**
** PARAMETERS:  value           - The value to be scaled down.
**              denominator     - The denominator
**
** RETURNS:     round(value/denominator)
*/
long MATHS_roundAndScale(long value, long denominator)
{
    return ((value + (denominator>>1))/denominator);
}
