/*!****************************************************************************
* @File          hostmsgproc.h
*
* @Title         UCCP common TV API wrapper
*
* @Date          15 April 2011
*
* @Copyright     Copyright (C) Imagination Technologies Limited
*
* @Description   Public interface to common TV API wrapper.
*
******************************************************************************/


#ifndef _HOSTMSGPROC_H_
#define _HOSTMSGPROC_H_
#include <uccrt.h>
#include "tvcore.h"

void IMGTV_MessageApp(UFW_COREDESC_T tvDescriptors[], unsigned numDesc, TV_ACTIVATION_PARAMETER_T *tunerList, bool initEnvironment);

bool IMGTV_activateReg(UFW_COREINSTANCE_T *activeInstance);

#endif /* _HOSTMSGPROC_H_ */
