/*
** FILE NAME:   $Source: /export/home/cvs/consumer_av/apps/saturn/bring_up/spim/thread0/code/sections_a.s,v $
**
** TITLE:       Section defn asm file.
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Define the location of the named data sections in the Bulk RAM.
**
**		Copyright (C) 2006, Imagination Technologies Ltd.
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/
#include "machine.inc"
/* Time deinterleaving data at start of bulk RAM */
$bulkbuffers SECTION DATA PARTIAL ORG 0xb0000000
$bulkbuffers ENDS
