/**
 * @file          virtualregister.h
 *
 * @brief         TV application control and status registers
 *
 * @date          10 Sep 2010
 *
 * TV applications are constructed in two parts: a "core" and a "wrapper".
 * Each TV standard's core has an API defined in terms of a set of virtual
 * registers and the values that are written to and read from them.
 *
 * It is possible
 * to associate a virtual interrupt with a virtual register and for such virtual
 * interrupts to be selectively masked by a register that is treated as an enable/mask
 * register.
 *
 * Groups of registers may  be synchronised to a master register. Writes to "mastered" registers are not
 * effective immediately, but are buffered and delayed until the master register is written.
 *
 * This register based model directs us to an API style for a standard's core
 * which resembles a hardware component.
 *
 * System specific wrapper programs transport register values and interrupts to
 * and from the controlling application.
 *
 * This module implements a register class, although as it's C and not C++
 * the register object methods appear as a set of function calls.
 *
 * Copyright (C) Imagination Technologies Limited 2010
 *
 */

#ifndef _VIRTUALREGISTER_H_
#define _VIRTUALREGISTER_H_

#include <stdint.h>
#include <stdbool.h>

/** Forward declaration of ::VREG_T object */
typedef struct _VREG_tag VREG_T;

/** Type definition for a register event handling function.
 *
 * @param[in] reg Pointer to register object that generated the event.
 * @param[in] parameter Handler parameter specified by ::VREG_installCoreHandler() or ::VREG_installWrapperHandler().
 * */
typedef void
VREG_HANDLER_T(VREG_T *reg, void *parameter);

/**
 * Private type definition for virtual register object
 */
struct _VREG_tag
{
    unsigned behaviour;
    VREG_HANDLER_T *coreHandler;
    VREG_HANDLER_T *wrapperHandler;
    void *coreParameter;
    void *wrapperParameter;
    uint32_t value;
    uint32_t effectiveValue;
    union
    {
        struct
        {
            VREG_T *master;
            VREG_T *masterLink;

        } valueReg;
        struct
        {
            VREG_T *maskLink;
            VREG_T *mask;
            uint32_t oldValue;
        } bitReg;
        struct
        {
            unsigned numValues;
            uint32_t *values;
        } multiReg;
    } reg;
};
/**
 * Read a register.
 *
 * In the case of a multi-valued register, the item with index zero is returned.
 *
 * @param[in]   reg     Pointer to register object
 * @returns             Register value
 */
uint32_t
VREG_read(VREG_T *reg);
/**
 * Write a register from the core.
 *
 * In the case of a multi-valued register, the item with index zero is written.
 *
 * @param[in]   reg     Pointer to register object
 * @param[in]   value   Register value
 */
void
VREG_coreWrite(VREG_T *reg, uint32_t value);
/**
 * Write a register from the wrapper.
 *
 * In the case of a multi-valued register, the item with index zero is written.
 *
 * @param[in]   reg     Pointer to register object
 * @param[in]   value   Register value
 */
void
VREG_wrapperWrite(VREG_T *reg, uint32_t value);
/**
 * Install an event handler in the wrapper application to respond to register updates.
 *
 * @param[in] reg       Pointer to register object
 * @param[in] handler   Pointer to handler function (or NULL to remove handler)
 * @param[in] handlerParameter Parameter to passed to handler.
 */
void
VREG_installWrapperHandler(VREG_T *reg, VREG_HANDLER_T *handler,
                           void *handlerParameter);
/**
 * Install an event handler in the core IP to respond to register updates.
 *
 * @param[in] reg       Pointer to register object
 * @param[in] handler   Pointer to handler function (or NULL to remove handler)
 * @param[in] handlerParameter Parameter to passed to handler.
 */
void
VREG_installCoreHandler(VREG_T *reg, VREG_HANDLER_T *handler,
                        void *handlerParameter);
/**
 * Initialise a register object for use as a simple, possibly "mastered", value register.
 *
 * Register initialisation can fail if a master register is itself mastered - mastering can not be chained.
 *
 * @param[in] reg Pointer to register object.
 * @param[in] master Pointer to register acting as synchronisation master for this register (or NULL)
 * @param[in] resetValue Register's initial value following ::VREG_initValue().
 * @returns true: Register initialised successfully. false: Register initialise failed
 */
bool
VREG_initValue(VREG_T *reg, VREG_T *master, uint32_t resetValue);
/**
 * Initialise a register object for use as a simple, possibly "masked", bit-set register.
 *
 * Register initialisation can fail if a mask register is itself masked - masking can not be chained.
 *
 * @param[in] reg Pointer to register object.
 * @param[in] mask Pointer to register acting as mask for this register (or NULL).
 * @param[in] resetValue Register's initial value following ::VREG_initBits().
 * @returns true: Register initialised successfully. false: Register initialise failed
 */
bool
VREG_initBits(VREG_T *reg, VREG_T *mask, uint32_t resetValue);
/**
 * Initialise a register object for use as a, possibly "masked", toggling bit-set register.
 * A toggling bit-set register differs from a simple bit-set register in that a value written
 * to the register is XORed with the previous value, rather than replacing it.
 *
 * Register initialisation can fail if a mask register is itself masked - masking can not be chained.
 *
 * @param[in] reg Pointer to register object.
 * @param[in] mask Pointer to register acting as mask for this register (or NULL).
 * @param[in] resetValue Register's initial value following ::VREG_initToggleBits().
 * @returns true: Register initialised successfully. false: Register initialise failed
 */
bool
VREG_initToggleBits(VREG_T *reg, VREG_T *mask, uint32_t resetValue);
/**
 * Initialise a register object as a multi-value register.
 *
 * Multi-value registers contain an array of values, rather than a single value. When you initialise
 * a multi-value register, you must provide a data buffer, to contain the array, as well as the ::VREG_T
 * object.
 *
 * Multi-value registers:\n
 * a) Can not be mastered or masked
 * b) Only call their event handlers when the first item (index zero) in the array is written
 *
 * @param[in] reg Pointer to register object.
 * @param[in] numValues Size of the register's valueArray
 * @param[in] valueArray Data array to hold the set of values associated with the register
 * @param[in] resetValue Register's initial value following ::VREG_initMultiValue(). This value is applied to all the
 * items in \c valueArray.
 */
void
VREG_initMultiValue(VREG_T *reg, unsigned numValues, uint32_t *valueArray,
                    uint32_t resetValue);
/**
 * Read indexed value from a multi-value register.
 *
 * This function is normally only used with multi-value registers. If it is called for other register types
 * it is equivalent to ::VREG_read() - \c index will be ignored.
 *
 * @param[in] reg Pointer to register object.
 * @param[in] index Value index.
 *
 * @returns Indexed register value.
 */
uint32_t
VREG_readIndexed(VREG_T *reg, unsigned index);
/**
 * Write indexed value to a multi-value register from the core.
 *
 * This function is normally only used with multi-value registers. If it is called for other register types
 * it is equivalent to ::VREG_coreWrite() - \c index will be ignored.
 *
 * @param[in] reg Pointer to register object.
 * @param[in] index Value index.
 * @param[in]   value   Register value.
 */
void
VREG_coreWriteIndexed(VREG_T *reg, unsigned index, uint32_t value);
/**
 * Write indexed value to a multi-value register from the wrapper.
 *
 * This function is normally only used with multi-value registers. If it is called for other register types
 * it is equivalent to ::VREG_wrapperWrite() - \c index will be ignored.
 *
 * @param[in] reg Pointer to register object.
 * @param[in] index Value index.
 * @param[in]   value   Register value.
 */
void
VREG_wrapperWriteIndexed(VREG_T *reg, unsigned index, uint32_t value);
/**
 * Get the size (number of value elements) of a register.
 *
 * This is normally used to retrieve the \c numValues argument passed to a call to ::VREG_initMultiValue().
 * For normal registers, the function always returns 1.
 *
 * @param[in] reg Pointer to register object.
 * @returns Number of values held in the register.
 */
unsigned
VREG_getValueCount(VREG_T *reg);
/**
 * Get a pointer to the effective data values held in a register
 *
 * This is normally used to retrieve a pointer to the data array associated with a multi-valued register.
 * This sometimes provides a more efficient way to read the entire contents of a multi-valued register.
 *
 * Writing to a multi-valued register via a pointer is <i>not recommended</i>, since to do so, would circumvent
 * the normal register event generation system.
 */
uint32_t *
VREG_getValuePointer(VREG_T *reg);

#endif /* _VIRTUALREGISTER_H_ */
