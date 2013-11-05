/*
** FILE NAME:   $RCSfile: core.c,v $
**
** TITLE:       Core function
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
** FUNCTION:    MATHS_core()
**
** DESCRIPTION: Core a value meaning that values smaller than a given threshold are forced to zero.
**
** PARAMETERS:  value   - Value to be cored
**              coreVal - The absolute coring threshold.
**
** RETURNS:     void
*/
long MATHS_core(long value, long coreVal)
{
    if ((value<coreVal) && (value > (-coreVal)))
        value = 0;

    return value;
}

