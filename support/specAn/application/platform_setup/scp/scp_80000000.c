/* Keep these first ... */
#ifdef METAG
#define METAG_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "plt_scp_setup.h"

#define ADC_FORMAT			(SCP_SAMPLE_2SCOMP)
/* Actually run at 78.64MHz, which is closest frequency achievable with the Saturn clock divider system */
#define INPUT_SAMPLE_RATE   (78640000)

/* Note that the AGC period register is 18 bits so maximum loadable value is 0x3ffff */
#define NORMAL_AGC_PERIOD	(0x3FFFF/4)	/* Normal AGC period in samples.  This gives a 0.725ms period. */
#define RAPID_AGC_PERIOD	(90000/4)	/* Rapid AGC period in samples.  This gives a 0.25ms period. */

/* The coefficients below have been generated using the SCP configuration Matlab tool
 * (cvs:/mobileTV/common/Matlab/simulation/PHY_SCP/SCP_configuration.m) with the following settings:
 *
 * scpConfig = [];
 * scpConfig.fsi               = [32e6, 40.96e6, 51.2e6, 64e6, 81.92e6] % one of these
 * scpConfig.fso               = 55e6;
 * scpConfig.sysClk            = 166e6;
 * scpConfig.fc                = 0;
 * scpConfig.bw                = 45e6;
 * scpConfig.maxPbRipple       = 5;
 * scpConfig.minSbAtten        = 10;
 * scpConfig.stopbandEdgeFreq  = 40e6;
 * scpConfig.tradeoff          = 'pb';
 * scpConfig.enablePbFlattening= 1;
 *
 * [ctxScp,scpConfig] = SCP_configuration(scpConfig);
 * [pbRipple,sbAtten] = SCP_performance(scpConfig);
 *
 * */
#define FIR_COEFFICIENTS_81M92  {    0,   -1,    2,    0,   -6,   10,   -6,  -12,   33,  -36,   -8,  278}	/* Fs=81.92MHz */
#define FIR_COEFFICIENTS_64M    {    0,    3,    3,   -5,   -8,    5,   18,    0,  -35,  -14,   91,  199}	/* Fs=64.0MHz  */
#define FIR_COEFFICIENTS_51M2   {   -5,  -10,   -5,    8,   17,   10,  -13,  -31,  -18,   35,  108,  161}	/* Fs=51.2MHz  */
#define FIR_COEFFICIENTS_40M96  {   -2,   -4,   -2,    5,   12,    7,  -11,  -29,  -19,   32,  106,  161}	/* Fs=40.96MHz */
#define FIR_COEFFICIENTS_32M    {  -14,  -14,   16,   21,  -19,  -28,   26,   43,  -38,  -76,   70,  270} 	/* Fs=32.0MHz  */

/* Note the decimation factors and resample factor will be over-ridden later so these ones don't matter. */
#define RESAMPLE_FACTOR		(0x80000000) /* This will be overwritten dependent on effective sample rate. */


TDEV_SCP_CONFIG_T pltScpConfig[SPECAN_SCP_RATE_MAX] =
{
//		{
//			ADC_FORMAT, 			/* ADC Format */
//			INPUT_SAMPLE_RATE,      /* Input data sample rate */
//			2,          			/* CIC decimation factor */
//			2,          			/* FIR decimation factor */
//			RESAMPLE_FACTOR,        /* Resampler value */
//			FIR_COEFFICIENTS_32M,   /* Narrow band FIR coefficients, normally used in acquisition */
//			FIR_COEFFICIENTS_32M,   /* Wide band FIR coefficients, normally used in normal operation */
//			RAPID_AGC_PERIOD,       /* rapid  AGC period in sample */
//			NORMAL_AGC_PERIOD       /* normal AGC period in sample */
//		},
//		{
//			ADC_FORMAT, 			/* ADC Format */
//			INPUT_SAMPLE_RATE,      /* Input data sample rate */
//			1,          			/* CIC decimation factor */
//			3,         				/* FIR decimation factor */
//			RESAMPLE_FACTOR,        /* Resampler value */
//			FIR_COEFFICIENTS_40M96, /* Narrow band FIR coefficients, normally used in acquisition */
//			FIR_COEFFICIENTS_40M96, /* Wide band FIR coefficients, normally used in normal operation */
//			RAPID_AGC_PERIOD,       /* rapid  AGC period in sample */
//			NORMAL_AGC_PERIOD       /* normal AGC period in sample */
//		},
		{
			ADC_FORMAT, 			/* ADC Format */
			INPUT_SAMPLE_RATE,      /* Input data sample rate */
			1,          			/* CIC decimation factor */
			3,          			/* FIR decimation factor */
			RESAMPLE_FACTOR,        /* Resampler value */
			FIR_COEFFICIENTS_51M2,  /* Narrow band FIR coefficients, normally used in acquisition */
			FIR_COEFFICIENTS_51M2,  /* Wide band FIR coefficients, normally used in normal operation */
			RAPID_AGC_PERIOD,       /* rapid  AGC period in sample */
			NORMAL_AGC_PERIOD       /* normal AGC period in sample */
		},
		{
			ADC_FORMAT, 			/* ADC Format */
			INPUT_SAMPLE_RATE,      /* Input data sample rate */
			1,          			/* CIC decimation factor */
			2,          			/* FIR decimation factor */
			RESAMPLE_FACTOR,        /* Resampler value */
			FIR_COEFFICIENTS_64M,   /* Narrow band FIR coefficients, normally used in acquisition */
			FIR_COEFFICIENTS_64M,   /* Wide band FIR coefficients, normally used in normal operation */
			RAPID_AGC_PERIOD,       /* rapid  AGC period in sample */
			NORMAL_AGC_PERIOD       /* normal AGC period in sample */
		},
		{
			ADC_FORMAT, 			/* ADC Format */
			INPUT_SAMPLE_RATE,      /* Input data sample rate */
			1,          			/* CIC decimation factor */
			1,          			/* FIR decimation factor */
			RESAMPLE_FACTOR,        /* Resampler value */
			FIR_COEFFICIENTS_81M92, /* Narrow band FIR coefficients, normally used in acquisition */
			FIR_COEFFICIENTS_81M92, /* Wide band FIR coefficients, normally used in normal operation */
			RAPID_AGC_PERIOD,       /* rapid  AGC period in sample */
			NORMAL_AGC_PERIOD       /* normal AGC period in sample */
		}
};
