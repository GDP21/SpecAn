/*!
 *****************************************************************************

 @file      scp.h
 @brief     Internal SCP functions and definitions

 Copyright (c) Imagination Technologies Limited. 2010

 ****************************************************************************/

#ifndef _SCP_H_
#define _SCP_H_
#include <stdbool.h>
#include <stdint.h>
#include "ucctypes.h"


/**
 * Initialise a SCP object.
 * Must be done before using the object in any other SCP_xx function.
 *
 * @param[in]   parentUCC       pointer to owner UCC object
 * @param[in]   scp             pointer to SCP object
 * @param[in]   scpNumber       SCP identifier (1..n)
 *
 * @return  true for success, false for failure.
 */
bool
_SCP_init(UCC_T *parentUCC, SCP_T *scp, unsigned scpNumber);

/* Common SCP interrupt handler */
void
_SCP_interruptHandler(unsigned hwstatAddr, unsigned extIntNumber, void *parameter);

#endif /* _SCP_H_ */
