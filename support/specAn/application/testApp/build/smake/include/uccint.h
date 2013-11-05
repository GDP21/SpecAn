/*!
 *****************************************************************************

 @file      uccint.h
 @brief     UCC interrupt handling

 Low level interrupt handling features. As a rule, these are not used directly by
 application authors, but are called by UCC device initialisation functions and by
 platform setup functions

 Copyright (c) Imagination Technologies Limited. 2010

 ****************************************************************************/

#ifndef _UCCINT_H_
#define _UCCINT_H_

#ifdef __META_FAMILY__
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif
#include <MeOS.h>

#include <stdbool.h>

/** Prototype for UCC interrupt handler
 * @param[in] hwstatAddr Address of the HWSTATEXT register containing the interrupt bit
 * @param[in] extIntNumber External interrupt number. (hwstatindex * 32) + hwstatbit
 * @param[in] parameter Arbitrary parameter to pass to the handler
 */
typedef void
_UCCINT_EXTINT_HANDLER_T(unsigned hwStatAddr, unsigned extIntNumber, void *parameter);

/** UCC interrupt handler dispatcher table entry */
typedef struct {
    /** Pointer to handler function */
    _UCCINT_EXTINT_HANDLER_T *isr;
    /** Parameter for handler function */
    void *parameter;
} _UCCINT_EXTINT_ENTRY_T;

/** UCC interrupt system structure */
typedef struct
{
    /** Vector of masks for active UCC interrupts */
    unsigned extIntMask[QIO_MAXVECGROUPS];
    /** vector values for active UCC interrupts */
    unsigned extVectorValue[QIO_MAXVECGROUPS];
    /** Pointer to UCC interrupt dispatcher table */
    _UCCINT_EXTINT_ENTRY_T *entryTab;
    /** Size of UCC interrupt dispatcher table */
    int numIsrs;
    /** Pointer to interrupt hardware descriptor */
    QIO_IVDESC_T *ivdPtr;
    /** true: Compact (MTX platforms) interrupt hardware. false: standard interrupt hardware. */
    bool compact;
    /** META/MTX signal number to use for UCC interrupts */
    int sigNum;
} _UCCINT_SYS_T;

/** UCC interrupt system descriptor structure */
typedef struct
{
	/** Interrupt system block */
	_UCCINT_SYS_T           intSystem;

    /** Number of entries in interrupt dispatch table */
	unsigned int            numDispatchEntries;

	/** Interrupt dispatch table */
	_UCCINT_EXTINT_ENTRY_T *dispatchTable;
} _UCCINT_SYS_DESC_T;

/**
 * Initialise the UCCRT's interrupt handling system. This is similar in concept to QIO_reset
 * except that:
 * 1) The dispatch table points to ISR functions rather than to device objects
 * 2) Direct wired devices are not supported
 *
 * The dispatch table must be big enough to accommodate the highest external hardware interrupt number.
 * The ivDesc parameter should be the same structure as is passed to QIO_reset.
 *
 * @param[in]   intSysDesc      pointer to descriptor for data used by UCC interrupt system
 * @param[in]   ivDesc          Interrupt vectoring hardware descriptor
 * @param[in]   sigNum          Signal number to use for non-QIO interrupts
 */
void
_UCCINT_init(_UCCINT_SYS_DESC_T *intSysDesc, QIO_IVDESC_T *ivDesc, unsigned sigNum);

/**
* Set up interrupt chaining by installing appropriate vector values and
* edge/level settings in the interrupt hardware system.
*
* @param[in]    parentStatusReg         Index of the "parent" (nearer the META/MTX) HWSTATEXT register
* @param[in]    parentStatusBit         Bit number of the parent HWSTATEXT register to which the child is connected
* @param[in]    childStatusReg         Index of the "child" HWSTATEXT register
* @param[in]    childVectorValue        Value to write into the child's vector register field to connect to the parent register bit
*/
void _UCCINT_chainInterrupts(int parentStatusReg, int parentStatusBit, int childStatusReg, int childVectorValue);


/**
 * Register a handler for an external interrupt with the primary dispatcher.
 *
 * The UCC runtime uses this function to register interrupt handlers for each UCC
 * in the system.
 *
 * Application authors may use this function to register handlers for non-UCC
 * devices that share the trigger used in the UCC run-time.
 *
 * External interrupts are numbered from 0..132 corresponding to bit 0 in the first HWSTATEXT
 * register up to bit 31 in the last (4th) HWSTATEXT register.
 *
 * @param[in]   extIntNumber     External Interrupt number
 * @param[in]   handler          Pointer to interrupt handler function (NULL to remove any installed handler)
 * @param[in]   handlerParameter Arbitrary parameter to be passed to the handler
 * @param[in]   latched          true: latching HWSTATEXT bit behaviour, false: non-latching HWSTATEXT bit behaviour
 *
 */
void
_UCCINT_registerExternalHandler(unsigned extIntNumber,
                               _UCCINT_EXTINT_HANDLER_T *handler, void *handlerParameter, bool latched);
#endif /* UCCINT_H_ */
