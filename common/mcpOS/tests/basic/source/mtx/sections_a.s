/*
** FILE NAME:   $Source: /cvs/uccToolkit/mcpOS/tests/basic/source/mtx/sections_a.s,v $
**
** TITLE:       Section definition asm file.
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Define the location of the named data sections
**
**              Copyright (C) 2010, Imagination Technologies Ltd.
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/
#include "machine.inc"

/* Section to hold RCD image */
$rcdimage SECTION DATA PARTIAL ORG 0xb402baac
$rcdimage ENDS

/* Section to hold DCP image */
$dcpimage SECTION DATA PARTIAL ORG 0xb402aaac
$dcpimage ENDS
