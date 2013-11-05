/*!****************************************************************************
 @File          SPECAN_core.h

 @Title         Spectrum Analyser core header file

 @Date          29 Nov 2012

 @Copyright     Copyright (C) Imagination Technologies Limited 2012

 @Description   Defines the public interface to the Spectrum Analyser core component.
 This follows a standard format consisting of a pointer to a "core descriptor"
 object and a set of virtual register name and value definitions

 ******************************************************************************/

#ifndef _SPECAN_CORE_H_
#define _SPECAN_CORE_H_

#include "uccrt.h"
#include "uccframework.h"
#include "tvcore.h"

#ifdef SPECAN_OUTPUT_8BIT
#define SPECAN_OUTPUT_FRAC_BITS	(0)
typedef int8_t SPECAN_DB_T;
#else
#define SPECAN_OUTPUT_FRAC_BITS	(8)
typedef int16_t SPECAN_DB_T;
#endif


/**
 * Pointer to core descriptor object. This contains the information
 * necessary for the framework to start up an instance of the core
 *
 */
extern UFW_COREDESC_T *SPECANdescriptor;

/*
 * Standard specific extensions to the list of register Ids.
 * These are numbered sequentially following the common register Ids
 */
enum
{
	SA_SCAN_RANGE = TV_REG_FIRST_STD_ID,
	SA_SCAN_RESOLUTION,
	SA_TUNING_STEP,
	SA_MEASUREMENT_CONTROL,
	SA_AVERAGING_PERIOD,
	SA_IF_GAIN_OVERRIDE,
	SA_TUNER_3DB_POINT,
	SA_TUNER_6DB_POINT,
	SA_TUNER_9DB_POINT,
	SA_TUNER_12DB_POINT,
	SA_REG_OUT_SPECTRUM_PTR,
	SA_OUT_SPECTRUM_LEN,
	SA_MAX_RSSI_REG,
	SA_REF_IF_GAIN_REG,
	SA_MAX_POWER_REG_0,
	SA_MAX_POWER_REG_1,
	SA_MAX_POWER_REG_2,
	SA_MAX_POWER_REG_3,
	SA_MAX_POWER_REG_4,
	SA_MAX_POWER_REG_5,
	SA_MAX_POWER_REG_6,
	SA_MAX_POWER_REG_7,
	SA_FAILURE_CODE,
	SPECAN_NUM_REGS
};

/*
 * Ennumerated values within API registers
 */

/* Ennumerate window function selection */
typedef enum
{
	SA_AVERAGING_PERIOD_N_2 = 0,
	SA_AVERAGING_PERIOD_N_4,
	SA_AVERAGING_PERIOD_N_8,
	SA_AVERAGING_PERIOD_N_16,
	SA_AVERAGING_PERIOD_N_32,
	SA_AVERAGING_PERIOD_N_64,
	SA_AVERAGING_PERIOD_N_128,
	SA_AVERAGING_PERIOD_N_256,
	SA_AVERAGING_PERIOD_N_512,
	SA_AVERAGING_PERIOD_N_1024,
	SA_AVERAGING_PERIOD_N_MAX
} SA_AVERAGING_PERIOD_N;

/* Failure codes */
typedef enum
{
	SA_SCAN_SIZE_EXCEEDS_AVAILABLE_MEMORY=1<<0,
	SA_SCAN_FAILURE_CODE_MAX
} SA_FAILURE_CODE_E;

/*
 * Ennumerated values within API registers
 */

/* Ennumerate window function selection */
typedef enum
{
	WINDOW_RECTANGULAR,
	WINDOW_HAMMING,
	WINDOW_HANNING
} SPECAN_WINDOW_T;

/* SCP configs should be created in platform setup for each of these sample rates. */
typedef enum
{
//    SPECAN_SCP_RATE_32M,		/* 32MHz */
//    SPECAN_SCP_RATE_40M96,	/* 40.96MHz */
    SPECAN_SCP_RATE_51M2,		/* 51.2MHz */
    SPECAN_SCP_RATE_64M,		/* 64MHz */
    SPECAN_SCP_RATE_81M92,		/* 81.92MHz */
    SPECAN_SCP_RATE_MAX
} SPECAN_SCP_RATE_E;

/*
 * Bit masks and bit shifts for fields within API registers
 */
#define SA_REG_AUTO_TUNE_STEP_BITSHIFT		(31)

/* SA_MEASUREMENT_CONTROL register shifts */
#define SA_REG_ENABLE_DC_COMP_BITSHIFT		(9)
#define SA_REG_ENABLE_DC_COMP_MASK			(0x1)
#define SA_REG_WINDOW_TYPE_BITSHIFT			(6)
#define SA_REG_WINDOW_TYPE_MASK 			(0x7)
#define SA_REG_SIGNAL_TYPE_BITSHIFT 		(3)
#define SA_REG_MAX_PEAK_WIDTH_BITSHIFT 		(0)
#define SA_REG_MAX_PEAK_WIDTH_MASK			(0x7)

/* SA_IF_GAIN_OVERRIDE register shifts */
#define SA_REG_OVERRIDE_IF_GAIN_BITSHIFT	(16)
#define SA_REG_IF_GAIN_BITSHIF				(0)

/* Output spectrum pointer. */
#define SA_REG_OUT_SPEC_MEM_TYPE_BITSHIFT	(30)
#define SA_REG_OUT_SPEC_PTR_MASK			(0x3FFFFFFF)

/* Spectral Power Peaks */
#define SA_MAX_POWER_N_BITSHIFT				(16)
#define SA_MAX_POWER_N_MASK					(0xFF)
#define SA_MAX_POWER_N_INDEX_BITSHIFT		(0)
#define SA_MAX_POWER_N_INDEX_MASK			(0xFF)

/*
 * Register counts - used to size register block allocation
 */
#define SPECAN_NUM_STDREG (SPECAN_NUM_REGS - TV_REG_FIRST_STD_ID)
#define SPECAN_TOTALREG (SPECAN_NUM_STDREG + TV_REG_NUM_COMMON_REG)

#define SPECAN_MAX_PEAK_COUNT ((SA_MAX_POWER_REG_7+1)-SA_MAX_POWER_REG_0)

/* For this application, in the documentation, we rename the state ACQUIRING to COMPLETED */
#define TV_STATE_COMPLETED	TV_STATE_ACQUIRING

#endif /* _SPECAN_CORE_H_ */
