/*!
*****************************************************************************

 @file      sections_a.s
 @brief     Define the location of the named data sections

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#include "machine.inc"
#include "uccStdSections_a.h"

/* Section to hold bulk buffers */
$bulkbuffers SECTION DATA PARTIAL ORG MEMGBL_DBL
$bulkbuffers ENDS
