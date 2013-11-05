/*!
*****************************************************************************

 @file      uccStdSections_a.h
 @brief     Define the location of the named data sections

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#include "uccBlockDefs.h"

/* Section to hold RCD image */
$rcdimage SECTION DATA PARTIAL ORG MEMGBL_DBL
$rcdimage ENDS

/* Section to hold DCP image */
$dcpimage SECTION DATA PARTIAL ORG MEMGBL_DBL
$dcpimage ENDS
