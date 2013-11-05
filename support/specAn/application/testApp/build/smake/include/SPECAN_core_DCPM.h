/* @cond DONT_DOXYGEN */
/**********************************************************************\

DCP linker output : library mode

This is an automatically generated file: DO NOT EDIT
Generated by DCP Linker 4.11.0

------------------------------------------------------------------------

Copyright (c) Imagination Technologies Limited
All rights reserved
Strictly confidential

\**********************************************************************/

#ifndef _SPECAN_CORE_DCPM_H_
#define _SPECAN_CORE_DCPM_H_

/*--------------------------------------------------------------------*/

#include "dcpTypes.h"

/*--------------------------------------------------------------------*/

/* Device indices */

enum
{
    DCP_deviceId_SCPDevice = 0,
    DCP_deviceId_irqGen0,
    DCP_deviceId_mcpos0
};

/*--------------------------------------------------------------------*/

/* Pipeline-instance indices */

enum
{
    DCP_pipelineId_SCPPipeline = 0
};

/*--------------------------------------------------------------------*/

/* Device-instance (use) indices */

/* SA_SCPPipeline */
enum
{
    DCP_SA_SCPPipeline_SCP = 0,
    DCP_SA_SCPPipeline_irqGen,
    DCP_SA_SCPPipeline_mcpos
};

/*--------------------------------------------------------------------*/

/* External declarations */

extern DCP_IMAGE_T DCP_image_default;

/*--------------------------------------------------------------------*/

#endif /* _SPECAN_CORE_DCPM_H_ */

/*--------------------------------------------------------------------*/
/* @endcond */
