#ifndef SA_SCP_DATA_H
#define SA_SCP_DATA_H

#include "uccrt.h"


/* This file is a simplified version of the Satelilte SCP configuration header. */

#define NUM_FIR_COEFFICIENTS     (12)
#define NUM_RESAMP_COEFFICIENTS  (13)

#define SA_SCP_FS_UNITY    (1<<25)

/* Our minimum symbol rate is determined by the maximum decimation factor available
    in the SCP, so is also dependent on IF clock rate.  However the resultant value is
    also floored on a defined minimum, below.
    The maximum total decimation factor is currently as follows:
      16 (CIC decimation)
     * 2 (FIR decimation)
     * 2 (DFE decimator)
     * 2 (DRM decimator)
     * 2 (resample factor)
         = 256
    However, set sampleRate / 245 in order to ensure RS factor isn't stuck on an endstop.
    (Gives minimum symbol rate of 320KBaud with 78.4MHz IF clock).
 */
#define MAX_SA_SCP_RESAMP_RATIO (245/SCP_SAMPLES_PER_SYMBOL) /* Fs_in / Fs_out */

/* Minimum resampler ratio based on IF clk of < (76.4MHz / (2*45MBd)), where 76.4MHz is a customer requirement. */
#define MIN_SA_SCP_RESAMP_RATIO ((uint64_t)0x1B0A3D7)      /* 0.845 in Q.25 */

#define GET_MIN_BAUD_RATE(Fs)  (Fs / (MAX_SA_SCP_RESAMP_RATIO*SCP_SAMPLES_PER_SYMBOL))

/* Our maximum symbol rate is determined by the need to avoid aliasing at the ADCs, so
    depends on the IF clock rate.  The baseband bandwidth of our signal will be
    (baudRate * 1.35)/2 (for 0.35 rolloff) and our sampling rate gives us a Nyquist freq
    of sampleRate/2; however we need to allow headroom between the signal bandwidth and
    Nyquist to allow for a) setting the tuner bandwidth wider, to allow margin for carrier
    frequency offset + filter flatness, and b) the fact that the tuner filter has sloppy rolloff
    and there may be significant energy in adjacent channels.
    Allow for 19/32 of sample rate, which allows us to run at 45MBaud with an 78.4MHz IF clk. */
//#define GET_MAX_BAUD_RATE(Fs)  ((Fs * 19) >> 5)
#define GET_MAX_BAUD_RATE(Fs)  ((unsigned)(((uint64_t)Fs<<25) / (MIN_SA_SCP_RESAMP_RATIO*SCP_SAMPLES_PER_SYMBOL)))

/* The SCP resampler coefficients. This will be extended to a set dependent on resampling ratio. */
/* Two sets - one for each FIR decimation. */
extern int16_t SA_scpResampCoeffs[2][NUM_RESAMP_COEFFICIENTS];

/* There are 2 back-end decimators available, the CIC decimator whose decimation factor is termed N, and
the FIR decimator whose decimation factor is termed P.  Each of these decimation factors can take a discreet
range of values, resulting in a set of total decimation factors which are available.  The set of possible unique
decimation factors is elaborated in this table along with the associated N and P values.
The entries are in ascending order of total decimation factor so the table can be searched to find the nearest
factor to the desired one. */
#define NUM_DECIMATION_FACTORS_AVAILABLE    (10)
#define NUM_CIC_ONLY_DECIMATION_FACTORS     (8)

/* As long as our final RS factor will not end up out of range, it is preferable to
stick with a FIR decimation factor of 1, since increasing this to 2 will give a poor
response (due to the limited coefficient set in the FIR filter which then has to deal
with a 2x oversampled signal) */

/* RS factor will end up less than 2 after applying decimation factor of 16 */
#define GET_NUM_DEC_FACTORS(RSFACTOR)   (((RSfactor / 16) < (2 << 25)) ? NUM_CIC_ONLY_DECIMATION_FACTORS : NUM_DECIMATION_FACTORS_AVAILABLE)

typedef enum
{
    SA_SCP_COEFF_SET_1=0,
    SA_SCP_COEFF_SET_2,
    SA_SCP_COEFF_SET_3,
    SA_SCP_COEFF_SET_4,
    SA_SCP_COEFF_SET_MAX
} SA_SCP_COEFF_SET_E;

typedef struct
{
    unsigned            decimation[NUM_DECIMATION_FACTORS_AVAILABLE];
    unsigned            firDec[NUM_DECIMATION_FACTORS_AVAILABLE];
    unsigned            cicDec[NUM_DECIMATION_FACTORS_AVAILABLE];

    SA_SCP_COEFF_SET_E coeffId[NUM_DECIMATION_FACTORS_AVAILABLE];
} DECIMATION_FACTOR_TABLE_T;

extern DECIMATION_FACTOR_TABLE_T SA_decimatorFactorTable;


typedef int16_t FIR_COEFF_SET_T[NUM_FIR_COEFFICIENTS];

/* Matrix of coefficient arrays. */
extern FIR_COEFF_SET_T *SA_FIRCoeffs[SA_SCP_COEFF_SET_MAX];

#endif /* SA_SCP_DATA_H */
