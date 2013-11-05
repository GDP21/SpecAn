/*!
 *****************************************************************************

 @file      mcp.h
 @brief     Internal MCP functions and definitions

 Copyright (c) Imagination Technologies Limited. 2010

 ****************************************************************************/

#ifndef _MCP_H_
#define _MCP_H_
#include <stdbool.h>
#include <stdint.h>
#include "ucctypes.h"

/**
 * Initialise a MCP object.
 * Must be done before using the object in any other MCP_xx function.
 *
 * @param[in]   parentUCC       pointer to owner UCC object
 * @param[in]   mcp             pointer to MCP object
 * @param[in]   mcpNumber       MCP identifier (1..4)
 *
 * @return  true for success, false for failure.
 */
bool
_MCP_init(UCC_T *parentUCC, MCP_T *mcp, unsigned mcpNumber);
/* Test mcp state */
bool
_MCP_haltedOnDebug(MCP_T *mcp);
/* Common MCP interrupt handler */
void
_MCP_interruptHandler(unsigned hwstatAddr, unsigned extIntNumber,
                      void *parameter);
/* low level MCP clock gate management - since these do read/modify/write
 * to the clock control registers, they are safe only when interrupts are disabled.
 * All return the previous value of the relevant clock control register.
 */
uint32_t
_MCP_enableClock(MCP_T *mcp);
uint32_t
_MCP_restoreClock(MCP_T *mcp, uint32_t oldClk);
uint32_t
_MCP_autogateClock(MCP_T *mcp);
uint32_t
_MCP_disableClock(MCP_T *mcp);
#endif /* _MCP_H_ */
