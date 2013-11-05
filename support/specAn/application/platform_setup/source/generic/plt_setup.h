/*!
*****************************************************************************
 
 @file   plt_setup.h
 @brief  Platform Setup

 Copyright (c) Imagination Technologies Limited.
 All Rights Reserved.
 Strictly Confidential.

****************************************************************************/

#ifndef PLT_SETUP_H
#define PLT_SETUP_H

#include "uccrt.h"

#ifdef INCLUDE_EMU_SETUP
/* Run-time setup for IMG emulator */

typedef enum
{
	ADC_FIFO_INPUT_MODE_REALIF, /* Real IF samples */
	ADC_FIFO_INPUT_MODE_IQ /* I/Q pairs */
} PLT_ADC_FIFO_INPUT_MODE;


typedef enum
{
	ADC_SAMPLE_FORMAT_2SCOMP, /* 2's complement */
	ADC_SAMPLE_FORMAT_OFFSET_BIN /* Offset binary */
} PLT_ADC_IF_SAMPLE_FORMAT;

typedef enum
{
	IF_CLK_SRC_18_286MHZ, /* 18.286MHz crystal */
	IF_CLK_SRC_32_768MHZ, /* 32.768MHz crystal */
	IF_CLK_SRC_ARBITRARY /* Arbitrary control via core clock division system */
} PLT_IF_CLK_SRC;

typedef enum
{
	CLK_1X,
	CLK_2X
} PLT_META_OR_LDPC_CLK_MULT;

/* Emulator configuration info */
typedef struct
{
	/* ADC FIFO input mode */
	PLT_ADC_FIFO_INPUT_MODE ADCinputMode;
	/* IF sample format */
	PLT_ADC_IF_SAMPLE_FORMAT IFsampleFormat;
	/* Oscillator clk src which will be divided down to form the IF clock. */
	PLT_IF_CLK_SRC IFclkSrc;
	/* IF clock divider.
	a) In modes other than IF_CLK_SRC_ARBITRARY, the clock source selected by IFclkSrc
	will be divided by IFclkDivider to form the IF clock.  The ratio must be a multiple of 2.
	When choosing this ratio, bear in mind that on the emulator the core clock is 20MHz
	and the 2x core clock (Meta, LDPC) is 40MHz.
	b) In IF_CLK_SRC_ARBITRARY mode the IF clock freq is clk_ref * IFclkMultiplier / IFclkDivider
	(Note core clock is fixed at clk_ref / 2) */
	unsigned IFclkDivider;
	unsigned IFclkMultiplier;
	/* Meta clock freq can be 1x or 2x core clock */
	PLT_META_OR_LDPC_CLK_MULT MetaClkRatio;
	/* LDPC clock freq can be 1x or 2x core clock */
	PLT_META_OR_LDPC_CLK_MULT LDPCclkRatio;
} PLT_EMU_CONFIG_T;

/* Emulator-specific setup function.  This does nothing if not building for an IMG emulator or FPGA-board target. */
void PLT_EMUsetup(PLT_EMU_CONFIG_T *EMUconfig);

#endif

/**
 * Structure that contains information about the platform. This is returned
 * by ::PLT_query.
 */
typedef struct plt_info_t
{
    /** String containing the name of the SOC */
    char *socName;

    /** String containing the name of the tuner */
    char *tunerName;

    /** Pointer to the tuner control structure */
    TUNER_USE_T *tuner;

    /** Flag describing if RF is in use. This really indicates whether a
     *  real or dummy tuner is in use.
     */
    bool rf;
} PLT_INFO_T;

/**
 * Sets up the platform.
 */
void PLT_setup(void);

/**
 * Returns information about the current platform, which is typically used to
 * perform platform specific run-time configuration.
 *
 * This function may only be called after the platform has been setup using
 * ::PLT_setup (an assert will be generated if this is not the case).
 *
 * @return  Pointer to platform information structure.
 */
PLT_INFO_T *PLT_query(void);


/**
 * This function is run within PLT_setup and performs the tuner part of
 * the setup, filling in the fields within info.  Its prototype is exported
 * here so that it can be run separately if required but this is not normally
 * done (just run PLT_setup).
 */
void PLT_setupTuner(PLT_INFO_T *info);


/* Some tuners have a tuning grid, i.e. they don't tuner to exactly the requested frequency.  Using this function,
the controlling software can find out, for the case of a local tuner, what frequency will be tuned to, for a given
requested tune frequency.  The freq argument and the return value are specified in Hz.  Note that really this information
should be embedded within the tuner driver, but at present there is no mechanism for passing
the information over the TDEV interface. */
unsigned PLT_getGriddedTunerFreq(unsigned freq);

#endif
