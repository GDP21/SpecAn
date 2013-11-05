/*!****************************************************************************
 * @File          tvreg.c
 *
 * @Title         TV core register access module
 *
 * @Date          7 Jan 2011
 *
 * @Copyright     Copyright (C) Imagination Technologies Limited 2012
 *
 * @Description   Provides a wrapper around the VREG_ function family to allow
 *                register access by ID when following the (discontinuous) register
 *                numbering scheme of TV cores.
 *                Deals with the fact that register Ids can't be used as simple indices
 *                into a core instance's register block
 *
 ******************************************************************************/

#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif
#include <stdint.h>
#include <stdbool.h>
#include "uccrt.h"
#include "tvcore.h"

uint32_t
TVREG_read(UFW_COREINSTANCE_T *coreInstance, uint32_t reg)
{
    return VREG_read(TVREG_ID2PTR(coreInstance, reg));
}

void
TVREG_coreWrite(UFW_COREINSTANCE_T *coreInstance, uint32_t reg, uint32_t value)
{
    if (reg > TV_REG_DEMOD_ID)
        VREG_coreWrite(TVREG_ID2PTR(coreInstance, reg), value);
}

void
TVREG_wrapperWrite(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                   uint32_t value)
{
    if (reg > TV_REG_DEMOD_ID)
        VREG_wrapperWrite(TVREG_ID2PTR(coreInstance, reg), value);
}

void
TVREG_installWrapperHandler(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                            VREG_HANDLER_T *handler, void *handlerParameter)
{
    VREG_installWrapperHandler(TVREG_ID2PTR(coreInstance, reg), handler,
                               handlerParameter);
}

void
TVREG_installCoreHandler(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                         VREG_HANDLER_T *handler, void *handlerParameter)
{
    VREG_installCoreHandler(TVREG_ID2PTR(coreInstance, reg), handler,
                            handlerParameter);
}

bool
TVREG_initValue(UFW_COREINSTANCE_T *coreInstance, uint32_t reg, uint32_t master,
                uint32_t resetValue)
{
    return VREG_initValue(
            TVREG_ID2PTR(coreInstance, reg),
            master == TV_REG_NULL_ID ? NULL :
                                       TVREG_ID2PTR(coreInstance, master),
            resetValue);
}

bool
TVREG_initBits(UFW_COREINSTANCE_T *coreInstance, uint32_t reg, uint32_t mask,
               uint32_t resetValue)
{
    return VREG_initBits(
            TVREG_ID2PTR(coreInstance, reg),
            mask == TV_REG_NULL_ID ? NULL : TVREG_ID2PTR(coreInstance, mask),
            resetValue);
}

bool
TVREG_initToggleBits(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                     uint32_t mask, uint32_t resetValue)
{
    return VREG_initToggleBits(
            TVREG_ID2PTR(coreInstance, reg),
            mask == TV_REG_NULL_ID ? NULL : TVREG_ID2PTR(coreInstance, mask),
            resetValue);
}

void
TVREG_initMultiValue(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                     unsigned numValues, uint32_t *valueArray,
                     uint32_t resetValue)
{
    VREG_initMultiValue(TVREG_ID2PTR(coreInstance, reg), numValues, valueArray,
                        resetValue);
}

uint32_t
TVREG_readIndexed(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                  unsigned index)
{
    return VREG_readIndexed(TVREG_ID2PTR(coreInstance, reg), index);
}

void
TVREG_coreWriteIndexed(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                       unsigned index, uint32_t value)
{
    if (reg > TV_REG_DEMOD_ID)
        VREG_coreWriteIndexed(TVREG_ID2PTR(coreInstance, reg), index, value);
}

void
TVREG_wrapperWriteIndexed(UFW_COREINSTANCE_T *coreInstance, uint32_t reg,
                          unsigned index, uint32_t value)
{
    if (reg > TV_REG_DEMOD_ID)
        VREG_wrapperWriteIndexed(TVREG_ID2PTR(coreInstance, reg), index, value);
}

unsigned
TVREG_getValueCount(UFW_COREINSTANCE_T *coreInstance, uint32_t reg)
{
    return VREG_getValueCount(TVREG_ID2PTR(coreInstance, reg));
}

uint32_t *
TVREG_getValuePointer(UFW_COREINSTANCE_T *coreInstance, uint32_t reg)
{
    return VREG_getValuePointer(TVREG_ID2PTR(coreInstance, reg));
}

