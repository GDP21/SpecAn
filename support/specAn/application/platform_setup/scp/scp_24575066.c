/* Keep these first ... */
#ifdef METAG
#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "plt_scp_setup.h"

#define ADC_FORMAT          (SCP_SAMPLE_2SCOMP)
#define INPUT_SAMPLE_RATE   (24576000)

#define AGCRAPID           (16384)     /* approx 500us*adc sampling - note DC loop works better if number is power of 2 */
#define NORMAL_AGC_PERIOD  (65536)     /* normal AGC period in sample (16384 * 4)*/

#define CICDECI_1P7MHZ    (8)   /* TODO */
#define FICDECI_1P7MHZ    (2)   /* TODO */
#define CICDECI_5MHZ      (3)
#define FICDECI_5MHZ      (2)
#define CICDECI_6MHZ      (2)
#define FICDECI_6MHZ      (2)
#define CICDECI_7MHZ      (2)
#define FICDECI_7MHZ      (2)
#define CICDECI_8MHZ      (2)
#define FICDECI_8MHZ      (2)

#define RESAMPLER_1P7MHZ  (0x6A8DF2B4)  /* TODO */
#define RESAMPLER_5MHZ    (0x5BBF35B1)
#define RESAMPLER_6MHZ    (0x72AF0325)
#define RESAMPLER_7MHZ    (0x624CDE20)
#define RESAMPLER_8MHZ    (0x5603425C)

//#define RESAMPLER_8MHZ    (0x56037ABA) //#8MHZ with 10ppm     (RESAMPLER_8MHZ * 1.000010)
//#define RESAMPLER_8MHZ    (0x5603B319) //#8MHZ with 20ppm     (RESAMPLER_8MHZ * 1.000020)
//#define RESAMPLER_8MHZ    (0x5603EB77) //#8MHZ with 30ppm     (RESAMPLER_8MHZ * 1.000030)
//#define RESAMPLER_8MHZ    (0x560423D6) //#8MHZ with 40ppm     (RESAMPLER_8MHZ * 1.000030)
//#define RESAMPLER_8MHZ    (0x56045C34) //#8MHZ with 50ppm     (RESAMPLER_8MHZ * 1.000030)
//#define RESAMPLER_8MHZ    (0x56049493) //#8MHZ with 60ppm     (RESAMPLER_8MHZ * 1.000030)

#define NARROW_FIR_COEFFS { -5, -3, 9, 9, -15, -17, 23, 33, -34, -68, 45, 205 }
#define WIDE_FIR_COEFFS   NARROW_FIR_COEFFS

TDEV_SCP_CONFIG_T pltScpConfig[SPECAN_SCP_RATE_MAX] =
{
    {   /* SCP Config for 8MHz Channel */
        ADC_FORMAT,                 /* ADC Format */
        INPUT_SAMPLE_RATE,          /* Input data sample rate */
        CICDECI_8MHZ,               /* CIC decimation factor */
        FICDECI_8MHZ,               /* FIR decimation factor */
        RESAMPLER_8MHZ,             /* Resampler value */
        NARROW_FIR_COEFFS,          /* Narrow band FIR coefficients, normally used in acquisition */
        WIDE_FIR_COEFFS,            /* Wide band FIR coefficients, normally used in normal operation */
        AGCRAPID,                   /* rapid  AGC period in sample */
        NORMAL_AGC_PERIOD           /* normal AGC period in sample */
    },
    {   /* SCP Config for 7MHz Channel */
        ADC_FORMAT,                 /* ADC Format */
        INPUT_SAMPLE_RATE,          /* Input data sample rate */
        CICDECI_7MHZ,               /* CIC decimation factor */
        FICDECI_7MHZ,               /* FIR decimation factor */
        RESAMPLER_7MHZ,             /* Resampler value */
        NARROW_FIR_COEFFS,          /* Narrow band FIR coefficients, normally used in acquisition */
        WIDE_FIR_COEFFS,            /* Wide band FIR coefficients, normally used in normal operation */
        AGCRAPID,                   /* rapid  AGC period in sample */
        NORMAL_AGC_PERIOD           /* normal AGC period in sample */
    },
    {   /* SCP Config for 6MHz Channel */
        ADC_FORMAT,                 /* ADC Format */
        INPUT_SAMPLE_RATE,          /* Input data sample rate */
        CICDECI_6MHZ,               /* CIC decimation factor */
        FICDECI_6MHZ,               /* FIR decimation factor */
        RESAMPLER_6MHZ,             /* Resampler value */
        NARROW_FIR_COEFFS,          /* Narrow band FIR coefficients, normally used in acquisition */
        WIDE_FIR_COEFFS,            /* Wide band FIR coefficients, normally used in normal operation */
        AGCRAPID,                   /* rapid  AGC period in sample */
        NORMAL_AGC_PERIOD           /* normal AGC period in sample */
    }
};
