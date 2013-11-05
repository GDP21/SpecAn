/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/msi_tuner/source/generic/msi_tuner.c,v $
**
** TITLE:       MSI tuner driver
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Implementation of MSI tuner driver
**
** NOTICE:      Copyright (C) Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#include <MeOS.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "uccrt.h"

#ifdef EN_DVBH_SUPPORT
#include "tuner_support.h"
#endif


#include "msi_tuner.h"
#include "msi_port.h"

#ifdef MSI_002
#include "msi_002.h"
#endif


#define RF_CONTROL_SIZE 3


/* Enum for the different bands the MSI001/2 supports,
** Note the order of these is important we assume we can index arrays with them */
typedef enum
{
    AM_BAND = 0,
    BAND_II,
    BAND_III,
    BAND_IV_V,
    L_BAND,
    NUM_BANDS
} FREQ_BAND_T;

/* Frequencies to be used as the boundaries between Bands.
** Not the actual edges of the bands but convenient points in between them to use as a threshold. */
#define BAND_II_LOWER_LIMIT     60000000
#define BAND_III_LOWER_LIMIT    150000000
#define BAND_IV_LOWER_LIMIT     300000000
#define L_BAND_LOWER_LIMIT      1000000000

#ifndef IF_RLCPLX
#define IF_RLCPLX    TRUE
#endif

#ifndef SPEC_INVERT
#define SPEC_INVERT   FALSE
#endif

#ifndef MSI_IF_FREQ
#define MSI_IF_FREQ       MSI_ZEROIF_IF_FREQ
#endif


#ifdef REF_FREQ_24M576
/* When using a reference frequency of 24.576MHz override some defaults */
#define	F_REF1					(24576000UL)    /* 24.576MHz in Hz */

/* When using this reference frequency change the default synth grid */
#ifndef SYNTH_GRID
#define	SYNTH_GRID				16000
#endif
#endif


/* The grid (synth steps) at the mixer in Hz.
** Use the same synth grid for all bands.
** The value is chosen to be easy to be derived from the reference frequency */
#ifndef SYNTH_GRID
#define SYNTH_GRID              13000
#endif

#ifndef UPDATE_MARGIN
#define UPDATE_MARGIN			SYNTH_GRID
#endif

#define AM_BAND_SYNTH_GRID      SYNTH_GRID
#define BAND_II_SYNTH_GRID      SYNTH_GRID
#define BAND_III_SYNTH_GRID     SYNTH_GRID
#define BAND_IV_V_SYNTH_GRID    SYNTH_GRID
#define L_BAND_SYNTH_GRID       SYNTH_GRID

/* The band specific Division ratios, the factor difference between
** the mixer frequency and the synthesizer frequency */
#define AM_BAND_DIVISION_RATIO      16
#define BAND_II_DIVISION_RATIO      32
#define BAND_III_DIVISION_RATIO     16
#define BAND_IV_V_DIVISION_RATIO    4
#define L_BAND_DIVISION_RATIO       2

/* The band specific LNA gain reductions */
#define AM_BAND_LNA_GR      18
#define BAND_II_LNA_GR      24
#define BAND_III_LNA_GR     24
#define BAND_IV_V_LNA_GR    7
#define L_BAND_LNA_GR       5

/* The band specific Mixer gain reductions */
#define AM_BAND_MIX_GR      19
#define BAND_II_MIX_GR      19
#define BAND_III_MIX_GR     19
#define BAND_IV_V_MIX_GR    19
#define L_BAND_MIX_GR       19

/* Maximum gains */
#define AM_BAND_MAX_GAIN    0
#define BAND_II_MAX_GAIN    105
#define BAND_III_MAX_GAIN   107
#define BAND_IV_V_MAX_GAIN  95
#define L_BAND_MAX_GAIN     105

#ifndef F_REF1
#define F_REF1              (26000000UL)    /* 26MHz in Hz */
#endif

#define F_REF_SCALE         (128)           /* Scale down by this value to fit the maths into 32 bits. */
                                            /* Note 26MHz/128 is an integer result. Hence no loss of precision */

#define	INVALID_GR		-1
#define MAX_BB_GR       59

#define HYSTERESIS_MARGIN   (2)
#define GR_HYSTERESIS_HI    (MAX_BB_GR)
#define GR_HYSTERESIS_LO    (GR_HYSTERESIS_HI - HYSTERESIS_MARGIN)


#define REGISTER_ADDRESS_MASK   0x0f

/* Register 0: IC mode / Power control */
#define AM_MODE             0x10
#define VHF_MODE            0x20
#define B3_MODE             0x40
#define B45_MODE            0x80U
#define BL_MODE             0x01
#define AM_MODE2            0x02
#define RF_SYNTH            0x04
#define AM_PORT_SELECT      0x08
#define FIL_MODE_SELECT0    0x10
#define FIL_MODE_SELECT1    0x20
#define FIL_BW_SEL0         0x40
#define FIL_BW_SEL1         0x80U
#define FIL_BW_SEL2         0x01
#define XTAL_SEL0           0x02
#define XTAL_SEL1           0x04
#define XTAL_SEL2           0x08
#define FMDEMODEN           0x10


/* Channel filter settings */
#define CHANNEL_FILTER_MODE_2048kHz     (0)
#define CHANNEL_FILTER_MODE_1620kHz     (FIL_MODE_SELECT0)
#define CHANNEL_FILTER_MODE_450kHz      (FIL_MODE_SELECT1)
#define CHANNEL_FILTER_MODE_LPF         (FIL_MODE_SELECT0 | FIL_MODE_SELECT1)

#define CHANNEL_FILTER_BWSEL01_200kHz   (0)
#define CHANNEL_FILTER_BWSEL2_200kHz    (0)

#define CHANNEL_FILTER_BWSEL01_300kHz   (FIL_BW_SEL0)
#define CHANNEL_FILTER_BWSEL2_300kHz    (0)

#define CHANNEL_FILTER_BWSEL01_600kHz   (FIL_BW_SEL1)
#define CHANNEL_FILTER_BWSEL2_600kHz    (0)

#define CHANNEL_FILTER_BWSEL01_1536kHz  (FIL_BW_SEL0 | FIL_BW_SEL1)
#define CHANNEL_FILTER_BWSEL2_1536kHz   (0)

#define CHANNEL_FILTER_BWSEL01_5MHz     (0)
#define CHANNEL_FILTER_BWSEL2_5MHz      (FIL_BW_SEL2)

#define CHANNEL_FILTER_BWSEL01_6MHz     (FIL_BW_SEL0)
#define CHANNEL_FILTER_BWSEL2_6MHz      (FIL_BW_SEL2)

#define CHANNEL_FILTER_BWSEL01_7MHz     (FIL_BW_SEL1)
#define CHANNEL_FILTER_BWSEL2_7MHz      (FIL_BW_SEL2)

#define CHANNEL_FILTER_BWSEL01_8MHz     (FIL_BW_SEL0 | FIL_BW_SEL1)
#define CHANNEL_FILTER_BWSEL2_8MHz      (FIL_BW_SEL2)


/* Crystal reference frequency settings */
#define XTAL_SEL_19M2       (0)
#define XTAL_SEL_22M        (XTAL_SEL0)
#define XTAL_SEL_24M576     (XTAL_SEL1)
#define XTAL_SEL_26M        (XTAL_SEL0 | XTAL_SEL1)
#define XTAL_SEL_38M4       (XTAL_SEL2)

#ifdef REF_FREQ_24M576
#define	XTAL_SEL			(XTAL_SEL_24M576)
#endif

/* If the XTAL we are using has not already been set, then default to 26MHz */
#ifndef XTAL_SEL
#define	XTAL_SEL			(XTAL_SEL_26M)
#endif

/* Register 1: Receiver gain control */
#define BBGAIN0             0x10
#define BBGAIN1             0x20
#define BBGAIN2             0x40
#define BBGAIN3             0x80U
#define BBGAIN3_0_MASK      0xF0U
#define BBGAIN4             0x01
#define BBGAIN5             0x02
#define BBGAIN5_4_MASK      0x03
#define MIXBU0              0x04
#define MIXBU1              0x08
#define MIXL                0x10
#define LNAGR               0x20
#define DCCAL0              0x40
#define DCCAL1              0x80U
#define DCCAL2              0x01
#define DCCAL_SPEEDUP       0x02

/* Register 2: Synthesizer programming */
#define FRAC3_0_MASK        0xF0U
#define FRAC11_4_MASK       0xFFU
#define INT5_0_MASK         0x3F
#define LNACAL_EN           0x40

/* Register 3: LO Trim Control */
#define AFC3_0_MASK         0xF0U
#define AFC11_4_MASK        0xFFU

/* Register 4: Auxiliary feature control */
#define SIGGEN_AMP3_0_MASK  0xF0U
#define SIGGEN_AMP7_4_MASK  0x0F
#define SIGGEN_EN           0x10

/* Register 5: RF Synthesizer Configuration */
#define THRESH3_0_MASK      0xF0U
#define THRESH11_4_MASK     0xFFU
#define REG5_RESERVED       0x28U

/* Register 6: DC Offset Calibration setup */
#define DCTRK_TIM3_0_MASK   0xF0U
#define DCTRK_TIM5_4_MASK   0x03
#define DCTRK_RATE5_0_MASK  0xFCU
#define DCTRK_RATE11_6_MASK 0x3F

/*
* Calibration factors to get the correct signal levels: this is effectively the gain difference between Emmy &
* Unicor ADC inputs. We want to drive the RF the same way. Note that there is 2dB hysterisis on the ADC
* gain.
*/
#ifdef UNICOR
#define FUDGE_DB_LNA_GR     (-11.0f)
#else
#define FUDGE_DB_LNA_GR     (0)
#endif
#define FUDGE_DB_LNA_GR_ON  (5.4771f - FUDGE_DB_LNA_GR)
#define FUDGE_DB_LNA_GR_OFF (5.4771f - FUDGE_DB_LNA_GR)


/* Define valid values of TRUE and FALSE that can be assigned to BOOLs */
#ifndef TRUE
#define TRUE    (1)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

/* Define NULL pointer value */
#ifndef NULL
#define NULL    ((void *)0)
#endif


#ifndef SPIM_OUTPUT_DMA_CHANNEL
#define SPIM_OUTPUT_DMA_CHANNEL	1
#endif


/* Static Data */

static const char bandsDivisionRatios[NUM_BANDS] =
{
    AM_BAND_DIVISION_RATIO,     /* AM Mode */
    BAND_II_DIVISION_RATIO,     /* VHF Mode */
    BAND_III_DIVISION_RATIO,    /* Band III Mode */
    BAND_IV_V_DIVISION_RATIO,   /* Band 4/5 Mode */
    L_BAND_DIVISION_RATIO       /* L-Band Mode */
};

/* The synthesizer step for each band (in Hz at the synthesizer so include division ratio) */
static const int bandsSynthStep[NUM_BANDS] =
{
    AM_BAND_SYNTH_GRID*AM_BAND_DIVISION_RATIO,      /* AM Mode */
    BAND_II_SYNTH_GRID*BAND_II_DIVISION_RATIO,      /* VHF Mode */
    BAND_III_SYNTH_GRID*BAND_III_DIVISION_RATIO,    /* Band III Mode */
    BAND_IV_V_SYNTH_GRID*BAND_IV_V_DIVISION_RATIO,  /* Band 4/5 Mode */
    L_BAND_SYNTH_GRID*L_BAND_DIVISION_RATIO         /* L-Band Mode */
};

/* The synthesizer step (THRESH) for each band (in units of the RF Sythesizer Configuration register) */
static const int bandsThresh[NUM_BANDS] =
{
    ((F_REF1 * 4)/(AM_BAND_SYNTH_GRID*AM_BAND_DIVISION_RATIO)),     /* AM Mode */
    ((F_REF1 * 4)/(BAND_II_SYNTH_GRID*BAND_II_DIVISION_RATIO)),     /* VHF Mode */
    ((F_REF1 * 4)/(BAND_III_SYNTH_GRID*BAND_III_DIVISION_RATIO)),   /* Band III Mode */
    ((F_REF1 * 4)/(BAND_IV_V_SYNTH_GRID*BAND_IV_V_DIVISION_RATIO)), /* Band 4/5 Mode */
    ((F_REF1 * 4)/(L_BAND_SYNTH_GRID*L_BAND_DIVISION_RATIO))        /* L-Band Mode */
};

static const int bandsLnaGR[NUM_BANDS] =
{
    AM_BAND_LNA_GR,     /* AM Mode */
    BAND_II_LNA_GR,     /* VHF Mode */
    BAND_III_LNA_GR,    /* Band III Mode */
    BAND_IV_V_LNA_GR,   /* Band 4/5 Mode */
    L_BAND_LNA_GR       /* L-Band Mode */
};

static const int bandsMixGR[NUM_BANDS] =
{
    AM_BAND_MIX_GR,     /* AM Mode */
    BAND_II_MIX_GR,     /* VHF Mode */
    BAND_III_MIX_GR,    /* Band III Mode */
    BAND_IV_V_MIX_GR,   /* Band 4/5 Mode */
    L_BAND_MIX_GR       /* L-Band Mode */
};

static int maxGains[NUM_BANDS] =
{
    AM_BAND_MAX_GAIN,
    BAND_II_MAX_GAIN,
    BAND_III_MAX_GAIN,
    BAND_IV_V_MAX_GAIN,
    L_BAND_MAX_GAIN
};


typedef enum
{
	SET_FREQ_MESSAGE = 0,
	SET_AGC_MESSAGE,
	POWER_UP_MESSAGE,
	POWER_DOWN_MESSAGE,
	POWER_SAVE_MESSAGE,
	NUMBER_OF_MSG		/* The number of different messages we can have */
} MSG_T;

typedef struct
{
	KRN_POOLLINK;
	MSG_T messageType;		/* What do we need to do with this message */
	TDEV_T *pTuner;			/* Tuner instance */
	unsigned messageFreq;		/* Frequency to tune to if it is a SET_FREQ_MESSAGE */
	TDEV_RF_PWRSAV_T messagePowerState;	/* Power saving state if it is a POWER_SAVE_MESSAGE */
	TDEV_AGCISR_HELPER_T * messageAgc;		/* AGC control info if it is a SET_AGC_MESSAGE */
	TDEV_COMPLETION_FUNC_T CompletionFunc;
	void *CompletionParameter;
} TUNER_MESSAGE_T;


	/* Size the pool to be more than the possible number of outstanding messages
	** incase we try and issue a new message just as we are completing one.
	** This assumes that only one of each message can occur at any time. */
#define MSG_POOL_SIZE	(NUMBER_OF_MSG + 3)
#define TASK_STACKSIZE (1024)


typedef struct
{
	UCC_STANDARD_T demodStandard;
    MSI_DVBH_MODE_T dvbhMode;

	/* Factor by which to ignore AGC updates. */
	int agcUpdateDecimateState;

	int useLnaGR;
	int useMixGR;
	int previousGr;
	/* Only Cal the L-Band once, so record the fact that we have. */
	int lBandCal;

    /* A record of the last selected band, so we don't have to configure this every time, only when it changes */
	FREQ_BAND_T selectedBand;
	long tunedFrequency;

	/* Bandwidth we are configured to operate at, in Hz.
	** Initialised to zero as an invalid value.
	** This will ensure that a bandwidth has been set before it is used. */
	long signalBandwidth;

	/* Buffer of bytes to send to RF device. Needs to be RF_CONTROL_SIZE + 1 because
	 * the MSI002 message format grows the MSI001 format by 1 byte.
	 */
#ifdef MSI_SCBM
	/* SCBM does not use DMA so this buffer can just be normal data */
	unsigned char rfCmdArray[MSI_RF_CONTROL_SIZE];
#endif
#ifdef MSI_SPIM
	/* SPIM uses DMA so this buffer needs to be in bulk memory. Given in MSI_configure */
	unsigned char *rfCmdArray = NULL;
#endif

	TUNER_MESSAGE_T messagePoolDesc[MSG_POOL_SIZE];
	KRN_POOL_T	 messagePool;
	KRN_MAILBOX_T taskMbox;
	int poolEmptyCount;

		/* Local copies of the AGC/SCP control info */
	TDEV_AGCISR_HELPER_T localAGCControl;
	TDEV_SCP_CONTROL_T localSCPControl;

	/* Task data */
	KRN_TASK_T task_tcb;
	unsigned int task_stack[TASK_STACKSIZE];
} MSI_WORKSPACE_T;





/* Bandwidths supported by this tuner */
static int supportedBws[] = {
    200000,
    300000,
    600000,
    1536000,
    5000000,
    6000000,
    7000000,
    8000000
};

/* Number of supported bandwidths */
#define NUM_SUPPORTED_BWS (sizeof(supportedBws)/sizeof(int))

/* Maximum tuner gains in BAND_IV_V for different supported bandwidths */
static const int band45MaxGains[NUM_SUPPORTED_BWS] =
{
    99, // bandwidth = 200KHz   // these values require verification from Mirics (13 Feb 09).
    99, // bandwidth = 300KHz
    99, // bandwidth = 600KHz
    99, // bandwidth = 1536KHz
    99, // bandwidth = 5MHz
    98, // bandwidth = 6MHz
    96, // bandwidth = 7MHz
    95  // bandwidth = 8MHz
};

static int bandwidthIndex = NUM_SUPPORTED_BWS;

#ifdef EN_DVBH_SUPPORT
static float dvbhLnaThreshs[] =
{
    -83,
    -83,
    -75
};
static float isdbtLnaThreshs[] =
{
    -83,
    -79,
    -71
};
#endif


static FREQ_BAND_T  Band(long freq);
static void MiricsMSI001_SetFreqBand(MSI_WORKSPACE_T *wrkspc, FREQ_BAND_T band);

/* Static Functions for Tuner API structure */
static TDEV_RETURN_T MSITuner_Init(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MSITuner_Reset(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MSITuner_SetFrequency(TDEV_T *pTuner, unsigned freq, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MSITuner_SetAGC(TDEV_T *pTuner, TDEV_AGCISR_HELPER_T * pControl, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MSITuner_Enable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MSITuner_Disable(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MSITuner_Configure(TDEV_T *pTuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MSITuner_Shutdown(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);

/* dummy functions for Tuner API structure */
static TDEV_RETURN_T MSITuner_initAGC(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MSITuner_readRFPower(TDEV_T *pTuner, TDEV_RSSI_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MSITuner_powerSave(TDEV_T *pTuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned control, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);
static TDEV_RETURN_T MSITuner_setIFAGCTimeConstant(TDEV_T *pTuner, int timeConstuS, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter);

TDEV_CONFIG_T MSITuner = {
    TDEV_VERSION_I32,         /* Version number - check that tuner and API are built with the same header */
    IF_RLCPLX,                     /* The IF interface is TRUE if Complex, FALSE if real */
    SPEC_INVERT,                   /* Set true if the IF spectrum is inverted - in this case IF is not inverted BUT this works around a suspected IQ swap */
    MSI_IF_FREQ,                   /* The final IF frequency of the tuner (Hz) (ZEROIF_IF_FREQ or LOWIF_XXXkHz_IF_FREQ) */
    SYNTH_GRID,                    /* The stepsize of the tuner PLL (Hz)      */
    UPDATE_MARGIN,				   /* The tuner PLL update margin (Hz)        */
    5000,                          /* Settling time in uS from power down to power up */
    5000,                          /* Settling time in uS from power save level 1 to power up */
    0,                             /* Settling time in uS from power save level 2 to power up */
    5000,                          /* Settling time in uS from tune to stable output  */
    MSITuner_Init,                 /* function */
    MSITuner_Reset,                /* function */
    MSITuner_Configure,            /* function */
    MSITuner_SetFrequency,         /* function */
    MSITuner_readRFPower,          /* dummy function */
    MSITuner_Enable,               /* function */
    MSITuner_Disable,              /* function */
    MSITuner_powerSave,            /* function */
    MSITuner_setIFAGCTimeConstant, /* dummy function */
    MSITuner_SetAGC,               /* function */
    MSITuner_initAGC,              /* dummy function */
    MSITuner_Shutdown              /* function */
};

/*
** Local resources
*/
#define WRITE(a,d)    (*(volatile unsigned long *)(a) = (d))
#define READ(a)       (*(volatile unsigned long *)(a))

#define FPGA_CONTROL_REGISTER   (0x04803000)
#define FPGA_CONTROL_RF_ENABLE  (1)

/*
** FUNCTION:    RFControlInit
**
** DESCRIPTION: This function initialises the resources used to control
**              access to the RF via SCB or SPI.
**
** INPUTS:      void
**
** RETURNS:     void
**
*/
void RFControlInit(TDEV_T *pTuner, MSI_WORKSPACE_T *wrkspc)
{
	MSI_CONFIG_T *config = pTuner->tunerConfigExtension;
    int success;

	(void)pTuner;
	(void)wrkspc;

    success = MSI_setupPort(config->dmacChannel, config->portNumber);
    if (!success)
    {
        __TBILogF("Port init failed\n");
        exit(-1);
    }

#ifdef AKBOARD
    WRITE(FPGA_CONTROL_REGISTER,FPGA_CONTROL_RF_ENABLE);
#endif
}

/*
** FUNCTION:    RFControlMessage
**
** DESCRIPTION: This function sends/gets bytes to/from SCB/SPI driver.
**
** INPUTS:      *buf         - pointer to array of bytes to be sent
**               size        - number of bytes in array
**               read_nwrite - read/write flag
**
** RETURNS:     void
**
*/
void RFControlMessage(unsigned char *buf, int size, int read_nwrite)
{
    int success;

    (void)read_nwrite;

    /* This driver builds messages in MSI 001 format (as historically this came
     * first), which is slighly different from the format required by MSI 002.
     * So to keep messages in a common format internally, reformat them before
     * being sent to the tuner on an 002 device.
     */
#ifdef MSI_002
    MSI_reformatMessage001To002(buf, &size);
#endif

    success = MSI_writeMessage(buf, size);
    if (!success)
    {
        __TBILogF("Port write failure\n");
        exit(-1);
    }
}


/*
** FUNCTION:    MiricsMSI001_LBandCal
**
** DESCRIPTION: Cal the L-Band LNA to given frequency by sending a string of bytes to the RF.
**
** RETURNS:     void
**
*/
static void MiricsMSI001_LBandCal(MSI_WORKSPACE_T *wrkspc, long freq)
{
    volatile int freqInt;
    volatile int freqFrac;
    int thresh = bandsThresh[L_BAND];

        /* Calc cal freq from tune freq */
    freq =  1385000000 + ((freq * 11) / 16);

    freqInt = freq / (F_REF1 * 4);
    freq -= freqInt * (F_REF1 * 4);
    freq /= 4 * F_REF_SCALE;    /* scale down the freq so to fit calculation into 32 bits, compensated by reducing the divisor in next line. */
    freqFrac = (thresh * freq)/(F_REF1/F_REF_SCALE);

        /* Register 2: Synthesizer programming */
    wrkspc->rfCmdArray[2] = 2 | ((freqFrac << 4) & FRAC3_0_MASK);
    wrkspc->rfCmdArray[1] = ((freqFrac >> 4) & FRAC11_4_MASK);
    wrkspc->rfCmdArray[0] = (freqInt & INT5_0_MASK) | LNACAL_EN;

        /* Write the RF Command Array to RF Device */
    RFControlMessage(wrkspc->rfCmdArray, RF_CONTROL_SIZE, FALSE);

    return;
}




/*
** FUNCTION:    HandleSetFreq
**
** DESCRIPTION: Send the strings of bytes to the RF, via the SCBM/SPI to set the carrier Frequency.
**
** RETURNS:     void
**
*/
void HandleSetFreq(MSI_WORKSPACE_T *wrkspc, long freq)
{
    unsigned long synthFreq;
    volatile int freqInt;
    volatile int freqFrac;
    FREQ_BAND_T band = Band(freq);

    int divisionRatio = bandsDivisionRatios[band];
//  int synthStep = bandsSynthStep[band];
    int thresh = bandsThresh[band];

	wrkspc->tunedFrequency = freq;

    freq = freq + MSITuner.frequencyIF; /* add I/F frequency - high side mix assuming frequencyIF is defined +ve */

    if (band != wrkspc->selectedBand)
    {
        MiricsMSI001_SetFreqBand(wrkspc, band);
        if ((band == L_BAND) && (!(wrkspc->lBandCal)))
        {
            /* Perform L-Band calibration at this frequency. */
            MiricsMSI001_LBandCal(wrkspc, freq);
            /* Set this so we only do the cal once. */
            wrkspc->lBandCal = TRUE;
        }
    }

    synthFreq = freq * divisionRatio;

    freqInt = synthFreq / (F_REF1 * 4);
    synthFreq -= freqInt * (F_REF1 * 4);
    synthFreq /= 4 * F_REF_SCALE;   /* scale down the freq so to fit calculation into 32 bits, compensated by reducing the divisor in next line. */
    freqFrac = (thresh * synthFreq)/(F_REF1/F_REF_SCALE);


    /* If mixFreq not on the thresh grid then we could use finer AFC control to set LO frequency.
    ** Currently we assume this is not necessary */

        /* Register 2: Synthesizer programming */
    wrkspc->rfCmdArray[2] = 2 | ((freqFrac << 4) & FRAC3_0_MASK);
    wrkspc->rfCmdArray[1] = ((freqFrac >> 4) & FRAC11_4_MASK);
    wrkspc->rfCmdArray[0] = freqInt & INT5_0_MASK;

        /* Write the RF Command Array to RF Device */
    RFControlMessage(wrkspc->rfCmdArray, RF_CONTROL_SIZE, FALSE);

    	/* We have just tuned, so allow next AGC update */
    wrkspc->previousGr = INVALID_GR;
    return;
}

/*
** FUNCTION:    selectLnaDefault
**
** DESCRIPTION: Default LNA selection
**
** RETURNS:     gr  Gain reduction value in dB
**
*/
static void selectLnaDefault(MSI_WORKSPACE_T *wrkspc, int gr, int LnaGain)
{
    if (gr < LnaGain)
    {
        wrkspc->useLnaGR = FALSE;
    }
    else if (gr > (LnaGain + HYSTERESIS_MARGIN))
    {
        wrkspc->useLnaGR = TRUE;
    }
}

#ifdef EN_DVBH_SUPPORT
/*
** FUNCTION:    selectLnaDvbh
**
** DESCRIPTION: LNA selection for DVBH
**
** RETURNS:     gain  Gain value in dB
**
*/
static volatile float msiRms;
static void selectLnaDvbh(MSI_WORKSPACE_T *wrkspc, int gain, TDEV_AGCISR_HELPER_T *pControl)
{
    float rms, thresh;

    /* Get RMS level of input signal */
    rms = TUNER_getRmsLevel(pControl);

    /* Subtract the requested gain to get the signal level at the tuner input */
    rms -= gain;

    if (useLnaGR)
    {
        rms += FUDGE_DB_LNA_GR_ON;
    }
    else
    {
        rms += FUDGE_DB_LNA_GR_OFF;
    }

    msiRms = rms;

    /* Lookup LNA threshold */
    switch (wrkspc->standard)
    {
	case UCC_STANDARD_DVBT:
	case UCC_STANDARD_DVBH:
	case UCC_STANDARD_DVBT2:
        thresh = dvbhLnaThreshs[wrkspc->dvbhMode];
        break;
    case UCC_STANDARD_ISDBT_1SEG:
        thresh = isdbtLnaThreshs[wrkspc->dvbhMode];
        break;
    case UCC_STANDARD_ISDBT_13SEG:
        thresh = isdbtLnaThreshs[wrkspc->dvbhMode];
        break;
    default:
        /* This shouldn't happen, but if it does, assert and use a sensible
         * default.
         */
        assert(0);
        thresh = -80;
        break;
    }


    /* Compare the RMS level against the threshold using hysteresis, and
     * update the LNA selection accordingly.
     */
    if (rms < thresh)
    {
        useLnaGR = FALSE;
    }

    if (rms > (thresh + HYSTERESIS_MARGIN))
    {
        useLnaGR = TRUE;
    }
}
#endif /* EN_DVBH_SUPPORT */



/*
** FUNCTION:    calculateGainReductionRequired
**
** DESCRIPTION: Calculate the gain reduction required as a result of the specified AGC control signal.
**
** RETURNS:     The gain reduction required in dB
**              note parameter *resultFractdB contains the fractional component (Q0.16) if non-null
**
*/
#define GR_FRACT_FRACTIONAL_BITS    (16)        /* Number of fractional bits in grFract value (note this must match TDEV_MAX_IF_GAIN range) */
#define GR_FRACT_ROUND              (1<<15)     /* Value to round calculations including grFract */
static int calculateGainReductionRequired(MSI_WORKSPACE_T *wrkspc, int IFgainValue, int *resultFractdB)
{
    FREQ_BAND_T band = wrkspc->selectedBand;
    int lnaGR;
    int mixerGR;
    int maxGr;
    int gr;


    /* If invalid band then assume typical band */
    if (band >= NUM_BANDS)
        band = BAND_III;

    lnaGR = bandsLnaGR[band];
    mixerGR = bandsMixGR[band];

    /* Calculate maximum gain reduction */
    maxGr = MAX_BB_GR + lnaGR + mixerGR;

    /* Calculate the gain reduction (1dB steps), rounding up */
    gr = (((TDEV_MAX_IF_GAIN - IFgainValue)*maxGr) + GR_FRACT_ROUND) >> GR_FRACT_FRACTIONAL_BITS;

    /* Also generate the fractional gain (fraction of 1dB step in Q0.16 format) */
    if (resultFractdB != NULL)
    {
        *resultFractdB = ((TDEV_MAX_IF_GAIN - IFgainValue)*maxGr) - (gr<<GR_FRACT_FRACTIONAL_BITS);
    }

    return gr;
}



/* Debug and test monitoring/control variables */
static int totalGr, grWithoutLna, grWithoutMixer, bbGr;

static volatile int ignoreAgc = 0;  /* Poke to 1 to disable AGC control */
static volatile int ForceAgc = 0;
static volatile int LogAgc = 0;
static volatile int forceLNAOff = 0;

/*
** FUNCTION:    HandleSetAGC
**
** DESCRIPTION: Send the strings of bytes to the RF, via the SPI to set the Gain.
**
** RETURNS:     void
**
*/
void HandleSetAGC(TDEV_T *pTuner, MSI_WORKSPACE_T *wrkspc, TDEV_AGCISR_HELPER_T *pControl)
{
	MSI_CONFIG_T *config = pTuner->tunerConfigExtension;
    int lnaGR;
    int mixerGR;
    int gr;
    int gain;
    int lnagr_mixl_bits;
    FREQ_BAND_T band = wrkspc->selectedBand;

	(void)pTuner;

	LogAgc = pControl->IFgainValue;

	if (ForceAgc != 0)
		pControl->IFgainValue = ForceAgc;

    /* If invalid band then assume typical band */
    if (band >= NUM_BANDS)
        band = BAND_III;


	/* When in normal operation (i.e. not rapid) only update one in every agcUpdateDecimateFactor updates. */
	if ((pControl->AGCMode == TDEV_NORMAL_AGC) && (--(wrkspc->agcUpdateDecimateState) > 0))
		return;

	wrkspc->agcUpdateDecimateState = config->agcUpdateDecimateFactor;

    if (ignoreAgc)
        return;

    gr = calculateGainReductionRequired(wrkspc, pControl->IFgainValue, NULL);
    totalGr = gr;   // Store in static variable for debug/logging only

    /* If the GR has not changed from previously then don't bother sending to the Tuner */
    if (gr == wrkspc->previousGr)
    	return;

    wrkspc->previousGr = gr;

    /* And calculate the resulting partiton of gain */
    lnaGR = bandsLnaGR[band];
    mixerGR = bandsMixGR[band];
    gain = maxGains[band] - gr;

    /* Perform LNA selection */
    switch (wrkspc->demodStandard)
    {
#ifdef EN_DVBH_SUPPORT
		case UCC_STANDARD_DVBT:
		case UCC_STANDARD_DVBH:
		case UCC_STANDARD_DVBT2:
            selectLnaDvbh(gain, pControl);
            break;
        case UCC_STANDARD_ISDBT_1SEG:
            selectLnaDvbh(gain, pControl);
            break;
        case UCC_STANDARD_ISDBT_13SEG:
            selectLnaDvbh(gain, pControl);
            break;
#endif /* EN_DVBH_SUPPORT */
        case UCC_STANDARD_FM:
        		/* For FM keep the SNR up at the expense of distortion. */
            selectLnaDefault(wrkspc,gr,GR_HYSTERESIS_LO);
            break;
        default:
        		/* For all other standards, do we only takeout the LNA GR when the signal is too small to do anything else */
//            selectLnaDefault(gr,lnaGR);
        		/* Or keep it as it is */
            selectLnaDefault(wrkspc,gr,GR_HYSTERESIS_LO);
            break;
    }

    /* Always enable LNA gain if the requested gain reduction is smaller than
     * the LNA gain reduction. This is needed to override bad decisions from
     * the signal level base scheme used in DVB-H.
     */
    if (gr < lnaGR)
    {
        wrkspc->useLnaGR = FALSE;
    }

    if (forceLNAOff)
        wrkspc->useLnaGR = TRUE;

    if (wrkspc->useLnaGR)
    {
        gr -= lnaGR;
        if (gr < 0)
            gr = 0;
        lnagr_mixl_bits = LNAGR;
    }
    else
    {
        /* else just use BB to implement gr */
        lnagr_mixl_bits = 0;
    }
    grWithoutLna = gr;

    if (gr < GR_HYSTERESIS_LO)
    {
        wrkspc->useMixGR = FALSE;
    }
    else if (gr > GR_HYSTERESIS_HI)
    {
        wrkspc->useMixGR = TRUE;
    }

    if (wrkspc->useMixGR)
    {
        gr -= mixerGR;
        if (gr < 0)
            gr = 0;
        lnagr_mixl_bits |= MIXL;
    }
    else
    {
        /* else just use BB to implement gr */
    }
    grWithoutMixer = gr;

    if (gr > MAX_BB_GR)
        gr = MAX_BB_GR;

    bbGr = gr;

        /* Register 1: Receiver gain control */
    wrkspc->rfCmdArray[2] = 1 | ((gr << 4) & BBGAIN3_0_MASK);
    wrkspc->rfCmdArray[1] = ((gr >> 4) & BBGAIN5_4_MASK) | lnagr_mixl_bits;
    wrkspc->rfCmdArray[0] = DCCAL2; /* DCCAL2 => one shot operation of DC Cal */

        /* Write the RF Command Array to RF Device */
    RFControlMessage(wrkspc->rfCmdArray, RF_CONTROL_SIZE, FALSE);

//__TBILogF("SetAFC called for gain %d gr %d\n", gain, gr); return;
}



#ifndef DISABLE_EARLY_GAIN_FRACTIONAL_DB_GAIN
/*
** FUNCTION:    updateScpEarlyGains
**
** DESCRIPTION: Trim out residual gain change requests smaller than the 1dB steps available from the Mirics RF
**
**              grFract is the residual fractional gain reduction requested, but not given
**              since the Mirics tuner is only capable of 1dB steps. This value is calculated here
**              and applied by modification of the SCP early gain values (available within
**              TDEV_SCP_CONTROL_T structure of ACG ISR helper structure).
**              Note this value is calculated here, but applied directly from the MSITuner_SetAGC
**              function since the AGC helper structure is data owned by the PHY and shouldn't be
**              accessed  after a delay within the MeOS controlled portion of the driver
**              The value is the fractional gain request in dB with Q0.16 format
*/
#define IQGAIN_ERR_SHIFT            (8)         /* Additional shift for maintaining fractional precision within calculations */
#define AGC_EARLY_GAIN_SHIFT        (8)         /* Nominal SCP early gain value (1<<8 = 256 => 0dB gain) */
#define SCP_EARLY_GAIN_1dB          (30)        /* Approximate additive value around 0dB gain control value for a 1dB gain variation */
#define SCP_FINE_GAIN_MIN_VALUE     (1)         /* Minimum allowable value for SCP fine gain */
#define SCP_FINE_GAIN_MAX_VALUE     (1023)      /* Maximum allowable value for SCP fine gain */
static volatile int disableGrFract = 0; /* Poke to 1 to disable compensation of fractional gain remainder */
void updateScpEarlyGains(MSI_WORKSPACE_T *wrkspc, TDEV_AGCISR_HELPER_T *pControl)
{
    int grFract = 0;
    int gainErr, nominalGain;

    if (disableGrFract)
        return;

    /* Evaluate fractional gain reduction requested */
    calculateGainReductionRequired(wrkspc, pControl->IFgainValue, &grFract);


    /* Work out what IQ gain imbalance is already being compensated by the SCP early fine gain control block */
    gainErr = ((1<<(IQGAIN_ERR_SHIFT+AGC_EARLY_GAIN_SHIFT))/pControl->pSCPcontrol->fineGainI - (1<<IQGAIN_ERR_SHIFT));

    /* Evaluate nominal gain with compensation for fractional gain reduction */
    nominalGain = (1<<AGC_EARLY_GAIN_SHIFT) - ((SCP_EARLY_GAIN_1dB * grFract + GR_FRACT_ROUND)>>GR_FRACT_FRACTIONAL_BITS);

    /* Evaluate new SCP early fine gain control values for I and Q channels to compensate both I/Q mismatch and fractional gain request from AGC */
    pControl->pSCPcontrol->fineGainI = (nominalGain<<(IQGAIN_ERR_SHIFT))/((1<<IQGAIN_ERR_SHIFT)+(gainErr));
    pControl->pSCPcontrol->fineGainQ = (nominalGain<<(IQGAIN_ERR_SHIFT))/((1<<IQGAIN_ERR_SHIFT)-(gainErr));

    /* Restrict the range of SCP control signals */
    if (pControl->pSCPcontrol->fineGainI < SCP_FINE_GAIN_MIN_VALUE)
        pControl->pSCPcontrol->fineGainI = SCP_FINE_GAIN_MIN_VALUE;
    if (pControl->pSCPcontrol->fineGainI > SCP_FINE_GAIN_MAX_VALUE)
        pControl->pSCPcontrol->fineGainI = SCP_FINE_GAIN_MAX_VALUE;
    if (pControl->pSCPcontrol->fineGainQ < SCP_FINE_GAIN_MIN_VALUE)
        pControl->pSCPcontrol->fineGainQ = SCP_FINE_GAIN_MIN_VALUE;
    if (pControl->pSCPcontrol->fineGainQ > SCP_FINE_GAIN_MAX_VALUE)
        pControl->pSCPcontrol->fineGainQ = SCP_FINE_GAIN_MAX_VALUE;

}
#endif




/*
** FUNCTION:    HandlePowerUp
**
** DESCRIPTION: Bring the RF out of power down.
**
** RETURNS:     void
**
*/
void HandlePowerUp(MSI_WORKSPACE_T *wrkspc)
{
	FREQ_BAND_T band = wrkspc->selectedBand;

	if (band == NUM_BANDS)
		band = BAND_IV_V;	/* If band unknown then set to a sensible band */

	MiricsMSI001_SetFreqBand(wrkspc, band);

	/* Enable next AGC update */
	wrkspc->previousGr = INVALID_GR;
	return;
}

/*
** FUNCTION:    HandlePowerDown
**
** DESCRIPTION: Power down the RF as much as we can
**
** RETURNS:     void
**
*/
void HandlePowerDown(MSI_WORKSPACE_T *wrkspc)
{
        /* Register 0: IC mode / Power Control */
    wrkspc->rfCmdArray[2] = 0;
    wrkspc->rfCmdArray[1] = CHANNEL_FILTER_MODE_LPF;
    wrkspc->rfCmdArray[0] = XTAL_SEL;

        /* Write the RF Command Array to RF Device */
    RFControlMessage(wrkspc->rfCmdArray, RF_CONTROL_SIZE, FALSE);

	return;
}

/*
** FUNCTION:    HandlePowerSave
**
** DESCRIPTION: Perform the required actions to put the RF in the requested power saving state
**
** RETURNS:     void
**
*/
void HandlePowerSave(MSI_WORKSPACE_T *wrkspc, TDEV_RF_PWRSAV_T powerSaveState)
{
	switch(powerSaveState)
	{
		case(TDEV_RF_PWRSAV_OFF):
				/* Full power on */
			HandlePowerUp(wrkspc);
			break;
		case(TDEV_RF_PWRSAV_LEVEL1):
				/* Same as level 2 */
			HandlePowerDown(wrkspc);
			break;
		case(TDEV_RF_PWRSAV_LEVEL2):
				/* Full power off */
			HandlePowerDown(wrkspc);
			break;
		default:
			break;
	}
	return;
}



/*
** FUNCTION:    taskMain
**
** DESCRIPTION: Simple dispatcher task to send Gain/Frequency requests to tuner via SCBM/SPIM device driver.
**
** RETURNS:     void
**
*/
static void taskMain(void)
{
	MSI_WORKSPACE_T *wrkspc = KRN_taskParameter(NULL);

    for(;;)
    {
            /* Wait for a message */
		TUNER_MESSAGE_T *msg = (TUNER_MESSAGE_T *)KRN_getMbox(&(wrkspc->taskMbox), KRN_INFWAIT);
        switch(msg->messageType)
        {
            case(SET_FREQ_MESSAGE):
                HandleSetFreq(wrkspc, msg->messageFreq);
                break;
            case(SET_AGC_MESSAGE):
                HandleSetAGC(msg->pTuner, wrkspc, msg->messageAgc);
                break;
            case(POWER_UP_MESSAGE):
            	HandlePowerUp(wrkspc);
                break;
            case(POWER_DOWN_MESSAGE):
            	HandlePowerDown(wrkspc);
                break;
            case(POWER_SAVE_MESSAGE):
            	HandlePowerSave(wrkspc, msg->messagePowerState);
                break;
            default:
                break;
        }
		msg->CompletionFunc(msg->pTuner, TDEV_SUCCESS, 	msg->CompletionParameter);
        	/* Finished with message so release back to pool */
        KRN_returnPool(msg);
    }
}

/*
** FUNCTION:    taskInit
**
** DESCRIPTION: Initialise and start task.
**
** RETURNS:     void
**
*/
static void taskInit(MSI_WORKSPACE_T *wrkspc)
{
	KRN_initPool(&(wrkspc->messagePool), wrkspc->messagePoolDesc, MSG_POOL_SIZE, sizeof(TUNER_MESSAGE_T));
	wrkspc->poolEmptyCount = 0;

	KRN_initMbox(&(wrkspc->taskMbox));

    KRN_startTask(taskMain, &(wrkspc->task_tcb), wrkspc->task_stack, TASK_STACKSIZE, 1, wrkspc, "Tuner Task");

    return;
}


/*
** FUNCTION:    Band
**
** DESCRIPTION: Returns the Band in which the given frequncy operates.
**
** RETURNS:     Band enumerated.
**
*/
static FREQ_BAND_T  Band(long freq)
{
    if (freq < BAND_II_LOWER_LIMIT)
    {
        assert(FALSE); /* Not handling AM */
        return(AM_BAND);
    }
    else if (freq < BAND_III_LOWER_LIMIT)
        return(BAND_II);
    else if (freq < BAND_IV_LOWER_LIMIT)
        return(BAND_III);
    else if (freq < L_BAND_LOWER_LIMIT)
        return(BAND_IV_V);
    else
        return(L_BAND);
}

/*
** FUNCTION:    MiricsMSI001_SetFreqBand
**
** DESCRIPTION: Configures the RF device for the given band of operation.
**
** RETURNS:     void
**
*/
static void MiricsMSI001_SetFreqBand(MSI_WORKSPACE_T *wrkspc, FREQ_BAND_T band)
{
    int thresh = bandsThresh[band];
    int modeBits0 = 0, modeBits1 = 0;
    int filModeSel = 0, filBWSel01 = 0, filBWSel2 = 0;

    /* Record this as the current Band */
    wrkspc->selectedBand = band;

        /* Register 5: RF Synthesizer Configuration */
    wrkspc->rfCmdArray[2] = 5 | ((thresh << 4) & THRESH3_0_MASK);
    wrkspc->rfCmdArray[1] = ((thresh >> 4) & THRESH11_4_MASK);
    wrkspc->rfCmdArray[0] = REG5_RESERVED;

        /* Write the RF Command Array to RF Device */
    RFControlMessage(wrkspc->rfCmdArray, RF_CONTROL_SIZE, FALSE);

    switch(band)
    {
        case(AM_BAND):
            modeBits0 = AM_MODE;
            break;
        case(BAND_II):
            modeBits0 = VHF_MODE;
            break;
        case(BAND_III):
            modeBits0 = B3_MODE;
            break;
        case(BAND_IV_V):
            modeBits0 = B45_MODE;
            maxGains[BAND_IV_V] = band45MaxGains[bandwidthIndex];
            break;
        case(L_BAND):
            modeBits1 = BL_MODE;
            break;
        default:
            assert(FALSE); /* Unsupported band */
            break;
    }

        /* Setup channel filter mode */
    switch(MSITuner.frequencyIF)
    {
        case(MSI_ZEROIF_IF_FREQ):
            filModeSel = CHANNEL_FILTER_MODE_LPF;       /* channel filter mode : LPF */
            break;
        case(MSI_LOWIF_2048kHz_IF_FREQ):
            filModeSel = CHANNEL_FILTER_MODE_2048kHz;   /* channel filter mode : BPF (2048kHz centre) */
            break;
        case(MSI_LOWIF_1620kHz_IF_FREQ):
            filModeSel = CHANNEL_FILTER_MODE_1620kHz;   /* channel filter mode : BPF (1620kHz centre) */
            break;
        case(MSI_LOWIF_450kHz_IF_FREQ):
            filModeSel = CHANNEL_FILTER_MODE_450kHz;    /* channel filter mode : BPF (450kHz centre) */
            break;
        default:
            assert(FALSE); /* Unsupported IF frequency */
            break;
    }

        /* Setup channel filter BW */
    switch(wrkspc->signalBandwidth)
    {
        case(200000):
            filBWSel01 = CHANNEL_FILTER_BWSEL01_200kHz;
            filBWSel2 = CHANNEL_FILTER_BWSEL2_200kHz;
            break;
        case(300000):
            filBWSel01 = CHANNEL_FILTER_BWSEL01_300kHz;
            filBWSel2 = CHANNEL_FILTER_BWSEL2_300kHz;
            break;
        case(600000):
            filBWSel01 = CHANNEL_FILTER_BWSEL01_600kHz;
            filBWSel2 = CHANNEL_FILTER_BWSEL2_600kHz;
            break;
        case(1536000):
            filBWSel01 = CHANNEL_FILTER_BWSEL01_1536kHz;
            filBWSel2 = CHANNEL_FILTER_BWSEL2_1536kHz;
            break;

        case(5000000):
            filBWSel01 = CHANNEL_FILTER_BWSEL01_5MHz;
            filBWSel2 = CHANNEL_FILTER_BWSEL2_5MHz;
            break;
        case(6000000):
            filBWSel01 = CHANNEL_FILTER_BWSEL01_6MHz;
            filBWSel2 = CHANNEL_FILTER_BWSEL2_6MHz;
            break;
        case(7000000):
            filBWSel01 = CHANNEL_FILTER_BWSEL01_7MHz;
            filBWSel2 = CHANNEL_FILTER_BWSEL2_7MHz;
            break;
        case(8000000):
            filBWSel01 = CHANNEL_FILTER_BWSEL01_8MHz;
            filBWSel2 = CHANNEL_FILTER_BWSEL2_8MHz;
            break;

        default:
            /* Unsupported bandwidth */

            /* If we don't know what the bandwidth is, use the widest
            ** and invalidate selected band to force going through here again, hopefully with the correct bandwidth
            */
            filBWSel01 = CHANNEL_FILTER_BWSEL01_8MHz;
            filBWSel2 = CHANNEL_FILTER_BWSEL2_8MHz;
            wrkspc->selectedBand = NUM_BANDS;
            break;
    }

        /* Register 0: IC mode / Power Control */
    wrkspc->rfCmdArray[2] = 0 | modeBits0;
    wrkspc->rfCmdArray[1] = modeBits1 | RF_SYNTH | filModeSel | filBWSel01; /* RF synth on, channel filter mode & B/W */
    wrkspc->rfCmdArray[0] = XTAL_SEL | filBWSel2;

        /* Write the RF Command Array to RF Device */
    RFControlMessage(wrkspc->rfCmdArray, RF_CONTROL_SIZE, FALSE);


    return;
}


/*
** FUNCTION:    MSITuner_SetFrequency
**
** DESCRIPTION: Send the set frequency request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MSITuner_SetFrequency(TDEV_T *pTuner, unsigned freq, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MSI_WORKSPACE_T *wrkspc = pTuner->workSpace;

	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *setFreqMsg = (TUNER_MESSAGE_T *) KRN_takePool(&(wrkspc->messagePool), KRN_NOWAIT);

	if (setFreqMsg == NULL)
	{
		wrkspc->poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
		return(TDEV_FAILURE);
	}

	setFreqMsg->messageFreq = freq;
	setFreqMsg->CompletionFunc = pCompletionFunc;
	setFreqMsg->CompletionParameter = completionParameter;
	setFreqMsg->messageType = SET_FREQ_MESSAGE;
	setFreqMsg->pTuner = pTuner;

	KRN_putMbox(&(wrkspc->taskMbox), setFreqMsg);

    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    MSITuner_Init
**
** DESCRIPTION: Initialises the Tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MSITuner_Init(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MSI_WORKSPACE_T *wrkspc = pTuner->workSpace;

	assert(MSI_TUNER_DRIVER_WORKSPACE_SIZE >= sizeof(MSI_WORKSPACE_T));

	/* Zero the context before we start */
	memset(wrkspc, 0, sizeof(MSI_WORKSPACE_T));

    /* Set to not a real band, so as to force correct init into required band */
    wrkspc-> selectedBand = NUM_BANDS;
    wrkspc->useLnaGR = FALSE;
    wrkspc->useMixGR = FALSE;
    wrkspc->lBandCal = FALSE;
    wrkspc->previousGr = INVALID_GR;
    wrkspc->demodStandard = UCC_STANDARD_NOT_SIGNALLED;
    wrkspc->dvbhMode = MSI_DVBH_QPSK;

    taskInit(wrkspc);
    RFControlInit(pTuner, wrkspc);

    pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);

    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    MSITuner_Shutdown()
**
** DESCRIPTION: Shutdown the tuner control - dummy function
*/
static TDEV_RETURN_T MSITuner_Shutdown(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MSI_WORKSPACE_T *wrkspc = pTuner->workSpace;

	KRN_removeTask(&(wrkspc->task_tcb));

	(void)MSI_shutdownPort();

	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
	return TDEV_SUCCESS;
}


/*
** FUNCTION:    MXL_DTV_Reset
**
** DESCRIPTION: Resets the Tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MSITuner_Reset(TDEV_T *pTuner, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return(TDEV_SUCCESS);
}


/*
** FUNCTION:    MSITuner_SetAGC
**
** DESCRIPTION: Send the gain request off to the tuner control task.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MSITuner_SetAGC(TDEV_T *pTuner, TDEV_AGCISR_HELPER_T * pControl, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MSI_WORKSPACE_T *wrkspc = pTuner->workSpace;
	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *setAGCMsg = (TUNER_MESSAGE_T *) KRN_takePool(&(wrkspc->messagePool), KRN_NOWAIT);

	if (setAGCMsg == NULL)
	{
		wrkspc->poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
		return(TDEV_FAILURE);
	}

    /* Take a local copy of these structures as we can't guarantee that they will still exist out of the scope of this function */
	wrkspc->localAGCControl = *pControl;
	wrkspc->localSCPControl = *(pControl->pSCPcontrol);
	wrkspc->localAGCControl.pSCPcontrol = &(wrkspc->localSCPControl);

	setAGCMsg->messageAgc = &(wrkspc->localAGCControl);
	setAGCMsg->CompletionFunc = pCompletionFunc;
	setAGCMsg->CompletionParameter = completionParameter;
	setAGCMsg->messageType = SET_AGC_MESSAGE;
	setAGCMsg->pTuner = pTuner;

	KRN_putMbox(&(wrkspc->taskMbox), setAGCMsg);

#ifndef DISABLE_EARLY_GAIN_FRACTIONAL_DB_GAIN
    /* Update SCP early gain to trim out gain changes less than the 1dB steps available in the Mirics RF */
    updateScpEarlyGains(wrkspc, pControl);
#endif

    return(TDEV_SUCCESS);
}

/*
** FUNCTION:    nearestBw()
**
** DESCRIPTION: Given an arbitrary bandwidth 'bw' find the nearest bandwidth supported by this tuner.
*/
static int nearestBw(int bw)
{
    int i, diff, minDiff = 0, minDiffInd = -1, firstDiff = 1;

    /* Find nearest bandwidth */
    for (i = 0; i < (int)NUM_SUPPORTED_BWS; i++)
    {
        diff = supportedBws[i] - bw;
        if ((diff >= 0) && (firstDiff || (diff < minDiff)))
        {
            minDiff = diff;
            minDiffInd = i;
            firstDiff = 0;
        }
    }

    /* Assert if the bandwidth hasn't been found.
     * Also do something safe in case asserts aren't enabled.
     */
    assert(minDiffInd != -1);
    if (minDiffInd == -1)
        minDiffInd = 0;

    bandwidthIndex = minDiffInd;

    return supportedBws[minDiffInd];
}

/*
** FUNCTION:    MSITuner_Configure()
**
** DESCRIPTION: This tuner uses MiricsMSI001_SetFrequency() to configure the tuner for a specific standard/bandwidth mode
*/
static TDEV_RETURN_T MSITuner_Configure(TDEV_T *pTuner, UCC_STANDARD_T standard, unsigned bandwidthHz, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MSI_WORKSPACE_T *wrkspc = pTuner->workSpace;
        /* Save bandwidth ready for when it is needed to configure RF. Need to align it to available tuner bandwidths. */
    wrkspc->signalBandwidth = nearestBw((int)bandwidthHz);
    if (wrkspc->selectedBand == BAND_IV_V)
        maxGains[BAND_IV_V] = band45MaxGains[bandwidthIndex];

    	/* Save the standard we are demodulating as we can treat some of them differently */
	wrkspc->demodStandard = standard;

    /* Set to not a real band, so as to force setting to required band that sets up the RF channel filter bandwidths */
    wrkspc->selectedBand = NUM_BANDS;

    pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}


/*
** FUNCTION:    MSITuner_Enable
**
** DESCRIPTION: Enable the tuner - currently only clears a few variables.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MSITuner_Enable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MSI_WORKSPACE_T *wrkspc = pTuner->workSpace;
	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *powerMsg = (TUNER_MESSAGE_T *) KRN_takePool(&(wrkspc->messagePool), KRN_NOWAIT);

	if (powerMsg == NULL)
	{
		wrkspc->poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
		return(TDEV_FAILURE);
	}

	powerMsg->CompletionFunc = pCompletionFunc;
	powerMsg->CompletionParameter = completionParameter;
	powerMsg->messageType = POWER_UP_MESSAGE;
	powerMsg->pTuner = pTuner;

	KRN_putMbox(&(wrkspc->taskMbox), powerMsg);

    //useLnaGR = FALSE;
    wrkspc->useMixGR = FALSE;
    wrkspc->lBandCal = FALSE;
	wrkspc->previousGr = INVALID_GR;

    (void)muxID;		/* Remove compile warning for unused arguments. */

    return(TDEV_SUCCESS);
}


/*
** FUNCTION:    MSITuner_Disable
**
** DESCRIPTION: Disable tuner.
**
** RETURNS:     Success
**
*/
static TDEV_RETURN_T MSITuner_Disable(TDEV_T *pTuner, unsigned muxID,TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MSI_WORKSPACE_T *wrkspc = pTuner->workSpace;
	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *powerMsg = (TUNER_MESSAGE_T *) KRN_takePool(&(wrkspc->messagePool), KRN_NOWAIT);

	if (powerMsg == NULL)
	{
		wrkspc->poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
		return(TDEV_FAILURE);
	}

	powerMsg->CompletionFunc = pCompletionFunc;
	powerMsg->CompletionParameter = completionParameter;
	powerMsg->messageType = POWER_DOWN_MESSAGE;
	powerMsg->pTuner = pTuner;

	KRN_putMbox(&(wrkspc->taskMbox), powerMsg);

	(void)muxID;		/* Remove compile warning for unused arguments. */

    return(TDEV_SUCCESS);
}


/*
** FUNCTION:    MSITuner_readRFPower()
**
** DESCRIPTION: This tuner does not have an RF power sense - dummy function
*/
static TDEV_RETURN_T MSITuner_readRFPower(TDEV_T *pTuner, TDEV_RSSI_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    pCompletionFunc(pTuner, 0, completionParameter);
    return TDEV_FAILURE;
}

/*
** FUNCTION:    MSITuner_powerSave()
**
** DESCRIPTION: This tuner does not have a power save mode, power it down - dummy function
*/
static TDEV_RETURN_T MSITuner_powerSave(TDEV_T *pTuner, TDEV_RF_PWRSAV_T powerSaveState, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MSI_WORKSPACE_T *wrkspc = pTuner->workSpace;
	/* To send a message to Tuner Task */
	TUNER_MESSAGE_T *powerMsg = (TUNER_MESSAGE_T *) KRN_takePool(&(wrkspc->messagePool), KRN_NOWAIT);

	if (powerMsg == NULL)
	{
		wrkspc->poolEmptyCount++;
		/* If we can't pass on message signal failure */
		pCompletionFunc(pTuner, TDEV_FAILURE, completionParameter);
		return(TDEV_FAILURE);
	}

	powerMsg->messagePowerState = powerSaveState;
	powerMsg->CompletionFunc = pCompletionFunc;
	powerMsg->CompletionParameter = completionParameter;
	powerMsg->messageType = POWER_SAVE_MESSAGE;
	powerMsg->pTuner = pTuner;

	KRN_putMbox(&(wrkspc->taskMbox), powerMsg);

    (void)(muxID);		/* Remove compile warnings for unused arguments. */

    return TDEV_SUCCESS;
}


/*
** FUNCTION:    MSITuner_setIFAGCTimeConstant()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static TDEV_RETURN_T MSITuner_setIFAGCTimeConstant(TDEV_T *pTuner, int timeConstuS, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
    (void)timeConstuS;	/* Remove compile warnings for unused arguments. */

    pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);
    return TDEV_SUCCESS;
}


/*
** FUNCTION:    MSITuner_initAGC()
**
** DESCRIPTION: The IF AGC is under software control! - dummy function
*/
static TDEV_RETURN_T MSITuner_initAGC(TDEV_T *pTuner, unsigned muxID, TDEV_COMPLETION_FUNC_T pCompletionFunc, void *completionParameter)
{
	MSI_WORKSPACE_T *wrkspc = pTuner->workSpace;

    (void)muxID;		/* Remove compile warnings for unused arguments. */
    pCompletionFunc(pTuner, TDEV_SUCCESS, completionParameter);

    wrkspc->previousGr = INVALID_GR;

    return TDEV_SUCCESS;
}
