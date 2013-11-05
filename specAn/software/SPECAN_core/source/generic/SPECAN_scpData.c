/*
 * SCP Resampler and FIR filter coefficients.
 *
 * This file is based on the Satellite equivalent.
 * There is scope to optimise the coefficients since channel filtering is not necessary,
 * in fact it is actively not wanted.
 *
 * Note that for the resampler set, the Kaiser window technique was used (see SCP_configuration.m).
 * */

#include "SPECAN_scpData.h"

int16_t SA_scpResampCoeffs[2][NUM_RESAMP_COEFFICIENTS] =
{
        /* Appropriate for all rates using FIR decimation of 1. */
        { 28,  10, -20, -53, -77, -80, -51,  14, 110, 225, 341, 437, 511 },

        /* SCP resampler coefficients, supplied by Adrian 9/9/2011.
         * Use for FIR decimation of 2. */
        { 2,   9,  22,  43,  72, 110, 155, 204, 254, 299, 335, 359, 367 }
};

DECIMATION_FACTOR_TABLE_T SA_decimatorFactorTable = {
        {1, 2, 3, 4, 6, 8, 12, 16, 24, 32},     /* decimation */
        {1, 2, 1, 1, 1, 1,  1,  1,  2,  2},     /* firDec */
        {1, 1, 3, 4, 6, 8, 12, 16, 12, 16},     /* cicDec */
        {
                SA_SCP_COEFF_SET_1,            /* decimation = 1 */
                SA_SCP_COEFF_SET_2,            /* decimation = 2 */
                SA_SCP_COEFF_SET_2,            /* decimation = 3 */
                SA_SCP_COEFF_SET_2,            /* decimation = 4 */
                SA_SCP_COEFF_SET_3,            /* decimation = 6 */
                SA_SCP_COEFF_SET_3,            /* decimation = 8 */
                SA_SCP_COEFF_SET_3,            /* decimation = 12 */
                SA_SCP_COEFF_SET_3,            /* decimation = 16 (untested) */
                SA_SCP_COEFF_SET_4,            /* decimation = 24 (untested) */
                SA_SCP_COEFF_SET_4             /* decimation = 32 */
        }
};

/* TODO. Generate coefficients for cic dec=1 */
FIR_COEFF_SET_T firCoeffsSet1 =
{
        /* FIR decimation=1, bw = 0.83Fout, sbEdge = Fout/2 */
        -9,    3,   12,   -2,  -17,   -1,   25,    8,  -38,  -25,   89,  211
};

/* Optimised for 51.2MHz Fout. cic dec=1, fir_dec=2*/
FIR_COEFF_SET_T firCoeffsSet2 =
{
		/* FIR decimation=1, bw = 0.83Fout, sbEdge = Fout/2 */
		-9,    3,   12,   -2,  -17,   -1,   25,    8,  -38,  -25,   89,  211
};

/* TODO. Generate coefficients for cic dec=6,8,12 */
FIR_COEFF_SET_T firCoeffsSet3 =
{
         -3,    6,   -1,   -8,    9,   -2,  -17,   31,   13,  -94,   14,  308
};


/* TODO. Generate coefficients for FIR decimation factors of 2. */
FIR_COEFF_SET_T firCoeffsSet4 =
{
        -1,    3,   -1,   -3,    4,   -3,   -6,   19,   -3,  -57,   46,  257
};


/* Matrix of coefficient sets for all sets and roll-off factors*/
FIR_COEFF_SET_T *SA_FIRCoeffs[SA_SCP_COEFF_SET_MAX] = {
        &firCoeffsSet1,
        &firCoeffsSet2,
        &firCoeffsSet3,
        &firCoeffsSet4
};
