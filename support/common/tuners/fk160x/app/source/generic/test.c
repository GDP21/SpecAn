/*
** FILE NAME:   $RCSfile: test.c,v $
**
** DESCRIPTION: Test application
**
**				Copyright (C)  Imagination Technologies Ltd.
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
*/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include "fk160x_tuner.h"

int main(void)
{
	return(0);
}



