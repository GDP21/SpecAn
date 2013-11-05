/*
** FILE NAME:   $RCSfile: restrict_range.c,v $
**
** TITLE:       Restrict Range - saturate
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
** FUNCTION:    MATHS_restrictRange()
**
** DESCRIPTION: Restrict the range of a value by saturating positive and negative.
**
** PARAMETERS:  value   - The value to restict the range of.
**              min     - The minumin allowable value.
**              max     - The maximum allowable value.
**
** RETURNS:     void
*/
long MATHS_restrictRange(long value, long min, long max)
{
    if (value < min)
        value = min;

    if (value > max)
        value = max;

    return value;
}

