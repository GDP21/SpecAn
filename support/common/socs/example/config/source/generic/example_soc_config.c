/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/socs/example/config/source/generic/example_soc_config.c,v $
**
** TITLE:       Example SOC config
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Interface for example SOC configuration driver
**
** NOTICE:		Copyright (C) 2009, Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
**				This example SOC Configuration driver is intended to be used as a template, a starting point for creating
**				SOC configuration code.
*/

#ifdef METAG
/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "example_soc_config.h"


void ExampleSOCConfig(void)
{
	return;
}
