/*
** FILE NAME:   $RCSfile: 2172_Driver.c,v $
**
** TITLE:       MAX2172 DAB/FM tuner driver
**
** AUTHOR:      Imagination Technologies
**
** DESCRIPTION: Implementation of a Maxim MAX2172 tuner driver
**
** NOTICE:      Copyright (C) Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/


/*
	Derived from example driver code from Maxim:
  Name: 2172_Driver.cpp
  Version 1.0.2
  Copyright: Maxim Integrated Products 2008
  Author: Paul Nichol
  Date: 1/15/09
  Description: Added LBand ROM table coefficients to the calibrated power measurements.
               Updated monigtored power equations and calibrated power equations.
               Fixed bug in the setting of the Master bias register from the ROM values.
               The masking of the bits was incorrect.  Added limits to RF_Frequency in
					SetTrackingFilter() to prevent overflow error.
					Fixed bug where LNA_BYP was not being reset after Max2172ReadPower() was executed.
*/


/* Keep these first ... */
#ifdef METAG
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif
#include <MeOS.h>
#include "2172_Driver.h"

#include "max_port.h"


/* Readability constants */
#define RF_FREQ_240MHZ	240000000
#define RF_FREQ_170MHZ	170000000
#define BAND3_RANGE		(RF_FREQ_240MHZ - RF_FREQ_170MHZ)

/* Make these limits just outside of the wanted bands, so tolerences don't give us problems at the very edges. */
#define	FM_BAND_UPPER_LIMIT  109000000
#define	L_BAND_LOWER_LIMIT	1450000000

#define WAIT_30MS	3

#define MAX2172_ADDR 0xc0         /* physical address of the Max2172 on the I2c bus */
#define XTALREF  24576000		  /* Frequency applied to Max2172 Xtal pin. */

#define MAX2172_NDIV 0            /* Register 0  - Ndivider */
#define MAX2172_FRAC2 0x1         /* Register 1  - Highest byte of Fractional N Diveder  */
#define MAX2172_FRAC1 0x2         /* Register 2  - Med byte of Fractional N Divider */
#define MAX2172_FRAC0 0x3         /* Register 3  - Lowest byte of Fractional N Divider */
#define MAX2172_TRK_FILT 0x4      /* Register 4  - Tracking Filter */
#define MAX2172_PLL_CONF 0x5      /* Register 5  - Pll control     */
#define MAX2172_MODE 0x6          /* Register 6  - Mode */
#define MAX2172_VCO_CONT 0x7      /* Register 7  - VCO Control */
#define MAX2172_RF_BBAND 0x8      /* Register 8  - BaseBand Control */
#define MAX2172_BIAS 0x9          /* Register 9  - Bias */
#define MAX2172_ROM_ADDR 0xa      /* Register 10 - Rom Address */
#define MAX2172_ROM_DATA 0xb      /* Register 11 - Rom Data */
#define MAX2172_TEST 0xc          /* Register 12 - Read-Only Test */
#define MAX2172_SHDN 0xd          /* Register 13 - Shutdown */
#define MAX2172_RSSI_TEMP 0xe     /* Register 14 - RSSI and Temperature Readback */
#define MAX2172_FM_FILT_RSSI 0xf  /* Register 15 - FM Filter and RSSI */
#define MAX2172_ROM_READ 0x10     /* Register 16 - Read-Only Rom Table Read Back */
#define MAX2172_STATUS 0x11       /* Register 17 - Read-Only Status Read Back */
#define MAX2172_AUTOTUNER 0x12    /* Register 18 - Read-Only Auto Tuner Read Back */

#define ROM_BIAS 0                /* Rom table reg 0 - Bias */
#define ROM_VHF_H 1               /* Rom table reg 1 - VHF H */
#define ROM_FM 2                  /* Rom table reg 2 - FM2 */
#define ROM_VHF_L 3               /* Rom table reg 3 - VHFL */
#define ROM_BB_BW 4               /* Rom table reg 4 - BB_BW */
#define ROM_FM_RSSI 5             /* /Rom table reg 4 - BB_BW */
#define ROM_DAB_RSSI 6            /* Rom table reg 4 - BB_BW */
#define ROM_FM_PD 7               /* Rom table reg 4 - BB_BW */


#define BAND_SEL_MASK	0x03

#define LNAGAIN 30


/* assign default values for each register */
static unsigned char NDiv = 0x37;
static unsigned char Frac2 = 0x05;
static unsigned char Frac1 = 0x6a;
static unsigned char Frac0 = 0xab;
static unsigned char Track_Filt = 0xc0;
static unsigned char PLL_Conf = 0x35;
static unsigned char Mode_Sel = 0xc2;
static unsigned char VCO_Cont = 0xbc;
static unsigned char BB_Cont = 0x98;
static unsigned char VCO_Bias = 0xa0;
static unsigned char ROM_Addr = 0x00;
static unsigned char ROM_Data = 0x00;
static unsigned char Test = 0x00;
static unsigned char Shdn = 0x00;
static unsigned char FM_Filt_RSSI = 0x8;


static int TFC2L;  /* C2 at Low end of band */
static int TFC1L;  /* C1 at Low end of band */
static int TFC2H;  /* C2 at High end of band */
static int TFC1H;  /* C1 at High end of band */
static int TFC2M;  /* C2 at FM band */
static int TFC1M;  /* C1 at FM band */

static int RSSIT_DAB; /* RSSI Trim for DAB Band */
static int RSSIT_FM;  /* RSSI Trim for FM Band */
static int RSSIT_LB;  /* RSSI Trim for L Band */
static int FBB_BW;    /* FM BB_BW from ROM */
static int FMPD_TH;   /* FM PD_TH from ROM */
static int DBB_BW;    /* DAB BB_BW from ROM */
static int DABPD_TH;  /* DAB PD_TH from ROM */


/* Function prototypes */
static int readAReg(int reg);
static unsigned char ReadRomTable(unsigned char TableIndex);
static void Load_PDTH_BBBW(int Band);
static void SetTrackingFilter(long L_RF_Frequency);
static void writeAReg(int reg, int value);

#ifdef NOT_USED
static int Max2172LockDetect(void);
static int Max2172ReadRSSI(void);
#endif

/* Read the contents of one of the ROM table rows
   You must first put the Rom Table row/index int MAX2172_ROM_ADDR to
   to the row index of the ROM table.  Then read back from register F.
   Bit D4 of MAX2172_ROM_ADDR must be set to 0, 0xe0 makes sure of this.
*/
static unsigned char ReadRomTable(unsigned char TableIndex)
{
   writeAReg(MAX2172_ROM_ADDR, (ROM_Addr & 0xe0) | TableIndex);   /* Write to Register */
   return readAReg(MAX2172_ROM_READ);
}



/*
	This Init routine must always be called before any I2C Communication can occur.
	It will initialize all the registers to their default values.
*/
void Max2172Init(void)
{

   writeAReg(MAX2172_NDIV, NDiv);   /* Write to Register */
   writeAReg(MAX2172_FRAC2, Frac2);   /* Write to Register */
   writeAReg(MAX2172_FRAC1, Frac1);   /* Write to Register */
   writeAReg(MAX2172_FRAC0, Frac0);   /* Write to Register */
   writeAReg(MAX2172_TRK_FILT, Track_Filt);   /* Write to Register */
   writeAReg(MAX2172_PLL_CONF, PLL_Conf);   /* Write to Register */
   writeAReg(MAX2172_MODE, Mode_Sel);   /* Write to Register */
   writeAReg(MAX2172_VCO_CONT, VCO_Cont);   /* Write to Register */
   writeAReg(MAX2172_RF_BBAND, BB_Cont);   /* Write to Register */
   writeAReg(MAX2172_BIAS, VCO_Bias);   /* Write to Register */
   writeAReg(MAX2172_ROM_ADDR, ROM_Addr);   /* Write to Register */
   writeAReg(MAX2172_ROM_DATA, ROM_Data);   /* Write to Register */
   writeAReg(MAX2172_TEST, Test);   /* Write to Register */
   writeAReg(MAX2172_SHDN, Shdn);   /* Write to Register */
   writeAReg(MAX2172_FM_FILT_RSSI, FM_Filt_RSSI);   /* Write to Register */

	/* The following routines read the values stored in the Max2172 ROM Table
	   The values read are kept in variables for later calculations.
		This prevents having to read from the ROM table for each operation
	*/

	/* Load VCO Bias and Master Bias values from ROM table, program bias register. */
	/* g_Reg(rgVCO_BIAS).MaskedDecimalValue(&H3F) = MaskN(ReadRomTable(ROM_VCO_BIAS), &H3F) */
	ROM_Data = ReadRomTable(ROM_BIAS) & 0x1F;   /* Set Index in ROM in table to read back Bias */
	ROM_Data |= (VCO_Bias & 0xE0);
	writeAReg(MAX2172_BIAS, ROM_Data);   /* Write to Register */

	/* Read the tracking filter cap values from ROM */
	ROM_Data = ReadRomTable(ROM_VHF_H);
	TFC2H = ROM_Data & 0x7;
	TFC1H = (ROM_Data & 0x38) >> 3;

	ROM_Data = ReadRomTable(ROM_VHF_L);
	TFC2L = ROM_Data & 0x7;
	TFC1L = (ROM_Data & 0x38) >> 3;

	ROM_Data = ReadRomTable(ROM_FM);
	TFC2M = ROM_Data & 0x7;
	TFC1M = (ROM_Data & 0x38) >> 3;


	RSSIT_FM = ReadRomTable(ROM_FM_RSSI);
	RSSIT_DAB = ReadRomTable(ROM_DAB_RSSI) & 0x1f;
	RSSIT_LB = ((ReadRomTable(ROM_BB_BW) & 0xc0) >> 3) | ((ReadRomTable(ROM_DAB_RSSI) &  0xe0) >>5);



	FBB_BW = (ReadRomTable(ROM_BB_BW) & 0x38) >> 3;   /* Get BB_BW bits from ROM: FBB_BW<2:0> */
	FMPD_TH = ReadRomTable(ROM_FM_PD) & 0x7;   /* Get FMPD_TH<2:0> bits from ROM */

	DBB_BW = ReadRomTable(ROM_BB_BW) & 0x7;   /* Get BB_BW bits from ROM: DBB_BW<2:0> */
	DABPD_TH = ReadRomTable(ROM_BIAS) & 0xE0;   /* Get DABPD_TH<2:0> bits from ROM */

}



/*
   As you tune in frequency in the VHF band, the tracking filter needs
   to be set as a function of frequency.  Stored in the ROM on the
	IC are two end points for the VHF frequency band.  One set of series
   and parallel capacitors for the 170MHz lower band edge and another
   set of series and parallel capicitor for the 240MHz or uppper band edge.
   This routine interpolates the needed series and parallel capacitor values
   from these two end points for the current frequency.
   Once the value is calculated, it is loaded into the Tracking Filter Caps
   register and the internal capacitors are switched in tuning the tracking
   filter.  For the FM band, the caps do not change as a function of frequency.
   This routine assumes that the Band Select has been set appropriately.
*/
static void SetTrackingFilter(long L_RF_Frequency)
{
	unsigned char Band;
	unsigned char TFC2 = TFC2M;
	unsigned char TFC1 = TFC1M;

	Band = Mode_Sel & BAND_SEL_MASK;  /* Get the band from the Mode Select register */

	switch (Band)
	{
		case 0:   /* VHF Band */
		{
			/* Interpolate the cap value between the VHF Low Cap and the VHF High Cap as */
			/* a ratio of the RF frequency between 170 and 240 MHz. + BAND3_RANGE/2 is to round up */
			/* Add everything up before dividing, so as too keep precision for as long as possible */
			if (L_RF_Frequency > RF_FREQ_240MHZ)
				L_RF_Frequency = RF_FREQ_240MHZ;
			else if (L_RF_Frequency < RF_FREQ_170MHZ)
				L_RF_Frequency = RF_FREQ_170MHZ;

			TFC2 = (unsigned char)((((L_RF_Frequency - RF_FREQ_170MHZ) * (TFC2H - TFC2L)) + (TFC2L * BAND3_RANGE) + (BAND3_RANGE/2)) / BAND3_RANGE);
			TFC1 = (unsigned char)((((L_RF_Frequency - RF_FREQ_170MHZ) * (TFC1H - TFC1L)) + (TFC1L * BAND3_RANGE) + (BAND3_RANGE/2)) / BAND3_RANGE);
			break;
		}
		case 1:  /* L Band */
			return;  /* No tracking filter for L band */
		case 2:
		{
			/* FM Band, No Interpolation one set of caps for all frequencies in band. */
			TFC2 = TFC2M;
			TFC1 = TFC1M;
			break;
		}
		case 3:  /* L Band  */
			return; /* No tracking filter for L band */
	}

		/* Maintain the top two bits of this register and add in the TFC */
	Track_Filt &= 0xc0;
	Track_Filt |= (TFC1 * 8) + TFC2;
	writeAReg(MAX2172_TRK_FILT, Track_Filt);

}

/******************************************************************************
   The routine Max2172SetLO will set the following registers based on the input Frequency (MHz)
      Rdiv = 2
      XtalRef = 24.576MHz    (default)
      if freq in VHF band then    VCO_Divider=16
                                  Band Select = VHF = 0 ( Reg 6 <0:1> )
                 LBAND then VCO_Divider=2
                            Band Select = LBAND = 1 ( Reg 6 <0:1> )
                 FM then VCO_Divider = 32
                            Band Select = FM = 2 ( Reg 6 <0:1> )

      Reg 0   NDiv = (int) VCO_Divider * Frequency * (Rdiv+1)) / (XtalRef * 2)
              FracN=modf(NpFN, &N) * 2^20
      Reg 1 <0:3> Highest 4 bits of FracN
      Reg 2 <0:7> Mid 8 bits of FracN
      Reg 3 <0:7> Lowest 8 bits of FracN
 ******************************************************************************/
void Max2172SetLO(long L_Frequency)
{
	unsigned long vcoFreq, remainder;
	long FRAC_N;
	int VCO_Divider = 1;
	int Rdiv, BAND_SEL = 0;

	Rdiv = ((PLL_Conf & 0x80) >> 7) + 1;  /* Retrieve R-Divider ( Reg5<7> )  */

	/* Sort out the band specific behaviour */
	if (L_Frequency > FM_BAND_UPPER_LIMIT && L_Frequency < L_BAND_LOWER_LIMIT)
	{	/* VHF  */
		BAND_SEL = 0;
		VCO_Divider = 16;
	}
	else if (L_Frequency >= L_BAND_LOWER_LIMIT)
	{	/* LBand  */
		BAND_SEL = 1;
		VCO_Divider = 2;
	}
	else if (L_Frequency <= FM_BAND_UPPER_LIMIT)
	{	/* FM band */
		long CenterFrequency = Max2172GetFMCenterFreq();

		BAND_SEL = 2;
		VCO_Divider = 32;

		L_Frequency += CenterFrequency;
	}

	/* Set BAND_SEL bits */
	Mode_Sel= (Mode_Sel & ~BAND_SEL_MASK) | BAND_SEL;

	Load_PDTH_BBBW(BAND_SEL);

	/* The vcoFreq is not just the true VCO freq, it is multiplied by Rdiv */
	vcoFreq = (L_Frequency * (unsigned long)((VCO_Divider * Rdiv)/2));

	/* Split out the integer and fractional portions */
	NDiv = (unsigned char) (vcoFreq / XTALREF);

	remainder = vcoFreq - (NDiv * XTALREF);

	/* FRAC_N = (2^20) * fractional remainder  */
	/* To keep this in range of the 32bit arithmetic divide through by 2^16 */
	FRAC_N = (long) ((remainder * (1<<4)) / (XTALREF/(1<<16)));

	Frac2 = (unsigned char)(Frac2 & 0xf0) | ((FRAC_N & 0xF0000) >> 16);
	Frac1 = (unsigned char)((FRAC_N & 0xFF00) >> 8);
	Frac0 = (unsigned char)FRAC_N & 0xFF;

	/* Note the order of the following register writes must be maintained. */
	writeAReg(MAX2172_NDIV, NDiv);
	writeAReg(MAX2172_FRAC2, Frac2);
	writeAReg(MAX2172_FRAC1, Frac1);
	writeAReg(MAX2172_FRAC0, Frac0);

	/* send band_sel bits: */
	writeAReg(MAX2172_MODE, Mode_Sel);

	SetTrackingFilter(L_Frequency);
}



static void Load_PDTH_BBBW(int Band)
{
	/* In Max2172Init() the  FBB_BW, FMPD_TH, DBB_BW, DABPD_TH were loaded from ROM.
	   These must be loaded into the RF and Baseband Control register (reg #8) for the
		band you are currently in.
		This only needs to be done if the band
		has changed from what it last was (and once initally).

		FM:
			ROM FBB_BW<2:0> goes into: register 8, BB_BW<2:0>
			ROM FMPD_TH<2:0> goes into: register 8, PD_TH<2:0>
		DAB:
			ROM DBB_BW<2:0> goes into: register 8, BB_BW<2:0>
			ROM DABPD_TH<2:0> goes into: register 8, PD_TH<2:0>

	*/

   static int LastBand = -1;

	if (LastBand != Band)
	{   /* Check to see if band has changed */
		LastBand = Band;
		if (Band == 2)
		{   /* FM Band */
			/* set the BB_BW<2:0> bits from the ROM's FBB_BW<2:0> bits.  */
			BB_Cont = (BB_Cont & 0xF8) | FBB_BW;
			/* Set the PD_TH<2:0> bit from the ROM's FMPD_TH<2:0> bits. */
			BB_Cont = (BB_Cont & 0x1F) | (FMPD_TH << 5);
		}
		else
		{
			/* set the BB_BW<2:0> bits from the ROM's DBB_BW<2:0> bits.  */
			BB_Cont = (BB_Cont & 0xF8) | DBB_BW;
			/* Set the PD_TH<2:0> bit from the ROM's DABPD_TH<2:0> bits. */
			BB_Cont = (BB_Cont & 0x1F) | DABPD_TH;
		}

		writeAReg(MAX2172_RF_BBAND, BB_Cont);   /* Write to Register */
	}
}



long Max2172GetFMCenterFreq(void)
{
	long returnFreq = 0;
 /* Create Center Frequency Table based on the FILTR<1:0> bits and */
 /* The BB_FC bit. Used only in the Max2172SetLO() routine */
	int FIL_TR = (Track_Filt & 0xc0) >> 5;
	int BB_FC = (VCO_Bias & 0x40) >> 6;
	int FINV_EN = (FM_Filt_RSSI & 4) >> 2;

	switch (FIL_TR + BB_FC)
	{
		case 0:
			returnFreq = 105000;
			break;
		case 1:
			returnFreq = 130000;
			break;
		case 2:
			returnFreq = 111000;
			break;
		case 3:
			returnFreq = 136000;
			break;
		case 4:
			returnFreq = 117500;
			break;
		case 5:
			returnFreq = 142500;
			break;
		case 6:
			returnFreq = 125000;
			break;
		case 7:
			returnFreq = 150000;
			break;
	}

		/* if bit clear then negate the frequency offset */
	if (FINV_EN != 1)
		returnFreq = -returnFreq;

	return(returnFreq);
}


/*
   Turn on or off the LNA bypass function.
	Signal levels above -70dBm should have it on
	for power measurements.  onoff = 1 = bypassed
	onoff = 0 = enabled.
*/
void Max2172LNA_BYP(int onoff)
{
	Mode_Sel = (Mode_Sel & 0xF7) | ((onoff & 1)<<3);
	writeAReg(MAX2172_MODE, Mode_Sel);
}
#ifdef NOT_USED


/*
  Does a latched read of the RSSI register.
  Returns the RSSI value
  Requires that the RSSI_EN = 0 (manual RSSI)

*/
static int Max2172ReadRSSI(void)
{
	int RSSI;
	writeAReg(MAX2172_ROM_ADDR, ROM_Addr | 0x80);  /* ADE Bit High (Enabled) */
	writeAReg(MAX2172_ROM_ADDR, ROM_Addr | 0x20);  /* ADL Bit High (latch) */

	RSSI = readAReg(MAX2172_RSSI_TEMP);

	writeAReg(MAX2172_ROM_ADDR, ROM_Addr &= 0x5f);  /* ADE, ADL Bits low */
	return RSSI;
}

/*
  Reads the Tune ADC value and determines if
  the PLL is locked.  Returns 1 for Locked,
  0 for unlocked.

*/
static int Max2172LockDetect(void)
{
	int ADC;
	writeAReg(MAX2172_VCO_CONT, VCO_Cont | 0x1);  /* ADE Bit High (Enabled) */
	writeAReg(MAX2172_VCO_CONT, VCO_Cont | 0x2);  /* ADL Bit High (latch) */

	ADC = readAReg(MAX2172_AUTOTUNER) & 0x7;  /* ADC Bits are the lower 3 */

	writeAReg(MAX2172_VCO_CONT, VCO_Cont &= 0xfc);  /* ADE, ADL Bits low */
	/* Loss of lock is indicated by 0 or 7 */
	if (ADC == 0 || ADC == 7)
		return 0;
	else
		return 1;
}
#endif

/*
    Sets the FINV_EN bit in register 15.
    The Up-Conversion Side-Band Selection bit.
    In FM Mode:
       mode = 0 = FM Channel above LO.
       mode = 1 = FM Channel below LO.
    In DAB Mode:
       mode = 0 = Non-Inverted.
       mode = 1 = Inverted.
    Requires resetting of LO since this effects the
    center frequency offset calculation.
*/
void Max2172Finv(int mode)
{
	FM_Filt_RSSI = (FM_Filt_RSSI & 0xfb) | (mode << 2);
	writeAReg(MAX2172_FM_FILT_RSSI,FM_Filt_RSSI);
}

/*
    Sets HIGH_LOW bit in register 7.
    The Up-Conversion Side-Band Selection bit.
    The sideband arguement must be 1 or 0.
    1 = HIGH-to-LOW, LOW-to-HIGH
    0 = HIGH-to-HIGH, LOW-to-LOW
    Requires resetting of LO since this effects the
    center frequency offset calculation.
*/
void Max2172HighLow(int sideband)
{
	VCO_Cont = (VCO_Cont & 0x7f) | (sideband << 7);
	writeAReg(MAX2172_VCO_CONT, VCO_Cont);
}




#ifdef USE_FP_MATH
#include <math.h>

static int Max2172MonitoredPower(double Frequency);
static int Max2172ReadPower(double Frequency, double Temperature);
static int Max2172ReadTemperature(void);

/*
   Reads the ADC, then uses the result along with the last temperature
   measurement to calculate the input power level.
	This is a calibrated power measurement.
	After done reading all power levels, call Max2172Init() again to
	return the unit to normal operating mode.
	If necessary for speed, this routine could be broken into 3 sections.
	Setup, Looping and measurement, Restore Settings.
*/
static int Max2172ReadPower(double Frequency, double Temperature)
{
	unsigned char BB_DET, RSSI_DET, TmpPLLConf, TmpMode_Sel;
	int PowerIn;
	int CalculatePower(double Frequency, double Temperature);

	/* Setup Section */

	/* Read RSSIT_FM<4:0> or RSSIT_DAB<4:0> from ROM, move
	upper two bits into RSSI_DET<1:0> reg 15, and lower 3 bits
	into BB_DET<2:0> reg 5.
	*/
	if ((Mode_Sel & 3) == 2)
	{   /* if band = FM */
		BB_DET = (RSSIT_FM & 7) << 2;
		RSSI_DET = RSSIT_FM & 0x18;
	}
	else if ((Mode_Sel & 2) == 1)
	{  /* if band = L Band */
		BB_DET = (RSSIT_LB & 7) << 2;
		RSSI_DET = RSSIT_LB & 0x18;
	}
	else
	{   /* DAB Mode */
		BB_DET = (RSSIT_DAB & 7) << 2;
		RSSI_DET = RSSIT_DAB & 0x18;
	}

	if (RSSI_DET  == 0)
	{
		/* if RSSIT_XX<4:3> = 0, then preset RSSI_DET<1:0> = 2, BB_DET<2:0> = 4
			otherwise, use the value in the ROM */
		writeAReg(MAX2172_FM_FILT_RSSI, (FM_Filt_RSSI & 0xe7) | 0x10);
		TmpPLLConf = (PLL_Conf & 0xe3) | 0x10;
	}
	else
	{
		/* move the bits read from ROM into the proper register: */
		writeAReg(MAX2172_FM_FILT_RSSI, (FM_Filt_RSSI & 0xe7) | RSSI_DET);
		TmpPLLConf = (PLL_Conf & 0xe3) | BB_DET;
	}


	/* Now Preset the Base band for a power measurement: */

	/* SHDN_EN = 1, SHDN_PD = 1 (SHDN_PD will not work if SHDN_EN = 0!) */
	writeAReg(MAX2172_SHDN, Shdn | 0x50);
	/* DDTH_EN, RSSI_EN, FAST_RSSI all = 1  */
	writeAReg(MAX2172_FRAC2, (Frac2 | 0xe0));
	/* Need to update reg(3) before reg(1) gets latched: */
	writeAReg(MAX2172_FRAC0, Frac0);
	/* DD_TH<1:0> = 1  */
	TmpPLLConf = (TmpPLLConf & 0xfc) | 1;
	writeAReg(MAX2172_PLL_CONF, TmpPLLConf);
	/* LNA_BYP = 0 (off)  */
	TmpMode_Sel = Mode_Sel;
	writeAReg(MAX2172_MODE, Mode_Sel &= 0xF7);


	/* Measurement Section */

	/*
	   This routine is designed to take a calibrated power measurement
		then return the Max2172 back to it's normal operating condition.

	   If the overhead of doing the setup, then restoration of settings
		is too long for the signal scan for each frequency, you can break this function
		into three sections and speed it up.  This depends on the speed
		of your microcontrollers I-squared-c serial clock.

		i.e.

		Setup Section
		for (f=88.1;f<=109;f+=.1){
			Max2172SetLO(f);
			PowerIn = CalculatePower(Frequency, Temperature);
			if (PowerIn > -72) {
				// Turn On LNA Bypass: LNA_BYP = 1 (on)
				writeAReg(MAX2172_MODE, Mode_Sel | 0x8);
				PowerIn = CalculatePower(Frequency, Temperature);
			}
		}
		Restore Section

	*/


	PowerIn = CalculatePower(Frequency, Temperature);

	/*  if the input power is greater than -72, bypass the LNA
		and re-measure the Input power.  Levels above -70
		require that the LNA is not used.  If the LNA were on,
		the measurement would be errant.
	*/

	if (PowerIn > -72)
	{
		/* Turn On LNA Bypass: LNA_BYP = 1 (on)  */
		writeAReg(MAX2172_MODE, Mode_Sel |= 0x8);
		PowerIn = CalculatePower(Frequency, Temperature);
	}

	/* Now restore to normal operating mode */

	writeAReg(MAX2172_MODE, Mode_Sel &= 0xf7);
	writeAReg(MAX2172_SHDN, Shdn);
	writeAReg(MAX2172_FRAC2, Frac2);
	/* Need to update reg(3) before reg(1) gets latched: */
	writeAReg(MAX2172_FRAC0, Frac0);
	writeAReg(MAX2172_PLL_CONF, PLL_Conf);
	writeAReg(MAX2172_MODE, TmpMode_Sel);
	writeAReg(MAX2172_FM_FILT_RSSI, FM_Filt_RSSI);

	return PowerIn;
}


static int CalculatePower(double Frequency, double Temperature)
{
	int PADC;
	int ADC;
	int PowerIn = 0;
	KRN_TASKQ_T queue;

	DQ_init(&queue);

	/* 30 ms delay. This allows the gain loop to stabilize. */
	KRN_hibernate(&queue, WAIT_30MS);

	ADC = readAReg(MAX2172_RSSI_TEMP) & 0x1f;

	if ((Mode_Sel & 3) == 2){   /* if band = FM */
		if (ADC >= 0x7)
		   PADC = -66 - (ADC - 0x7) * 2;
		else if (ADC <= 0x7)
			PADC = -66 + (0x7 - ADC) * 5;
      if (Temperature > 31)
         PowerIn = (int)(PADC + (1 / 20) * (Temperature - 31));
		else if (Temperature <= 31)
			PowerIn = (int)(PADC + (1 / 35) * (Temperature - 31));
		if ((Mode_Sel & 0x8) != 0) /* if LNA is bypassed */
			PowerIn += 30;
	}
	else if ((Mode_Sel & 3) == 1){  /* if band = L Band */
		if (ADC >= 0x7)
		   PADC = -57 - (ADC - 0x7) * 2;
		else if (ADC < 0x7)
			PADC = -57 + (0x7 - ADC) * 5;
		if (Temperature > 31)
			PowerIn = (int)PADC + (1/20-0.8/50) * (Temperature - 31);
		else
			PowerIn = (int)PADC + (1 / 35) * (Temperature - 31);
		if ((Mode_Sel & 0x8) != 0) /* if LNA is bypassed */
			PowerIn += 22;
	}
	else{   /* DAB Mode */
		if (ADC >= 0x7)
		   PADC = -58 - (ADC - 0x7) * 2;
		else if (ADC < 0x7)
			PADC = -58 + (0x7 - ADC) * 5;
      if (Temperature > 31)
         PowerIn = (int)(PADC + (1 / 20) * (Temperature - 31));
		else if (Temperature <= 31)
			PowerIn = (int)(PADC + (1 / 35) * (Temperature - 31));
		if ((Mode_Sel & 0x8) != 0) /* if LNA is bypassed */
			PowerIn += 21;
		PowerIn += (Frequency-200)/30;
	}
	return PowerIn;
}

/*
    This does a less accurate power measurement than
	 Max2172ReadPower() but does not disrupt the AGC
	 loop (and thus does not effect the signal coming in).
    Returns power in dBm.  This is function uses 5 bits from the BBAGC and 5 bits
	 from the RFAGC.
*/
static int Max2172MonitoredPower(double Frequency)
{
   /* Requires FAST_RSSI = 0, RSSI_EN = 1  */
   int rfagc;
   int bbagc;
   int det_th;
   int det_gain_r;
   int lna_status;
   double ctgain;
   double rfgain;
   double bbgain;
   double det_gain;

	/* RSSI_EN = 0 (Manual ADC update). */
   writeAReg(MAX2172_FRAC2, Frac2 & 0xbf);
	/* Need to update reg(3) before reg(1) gets latched: */
   writeAReg(MAX2172_FRAC0, Frac0);

	writeAReg(MAX2172_FM_FILT_RSSI, (FM_Filt_RSSI & 0xfc) | 2);   /* Set RFAGC path (RSSI_SEL) */
   rfagc = Max2172ReadRSSI() & 0x1f;
	writeAReg(MAX2172_FM_FILT_RSSI, (FM_Filt_RSSI & 0xfc) | 3);   /* Set BBAGC path (RSSI_SEL) */
   bbagc = Max2172ReadRSSI() & 0x1f;

	writeAReg(MAX2172_FM_FILT_RSSI, FM_Filt_RSSI);   /* Restore RSSI_SEL to previous state. */
   writeAReg(MAX2172_FRAC2, Frac2);   /* Restore previous RSSI_EN state. */
	/* Need to update reg(3) before reg(1) gets latched: */
   writeAReg(MAX2172_FRAC0, Frac0);

   /*   read the LNA_BYP status 1=on, 0=off */
   lna_status = ((Mode_Sel & 8) >> 3);  /* 1 = LNA off, 0 = LNA on */

	if ((Mode_Sel & 3) == 2)
	{   /* if band = FM  */
		if (rfagc < 9)
			rfagc = 9;
		if (rfagc > 19)
			rfagc = 19;
      /*   ct gain of the tuner */
      ctgain = 7.2 - lna_status * 30;
      /*  slope = 38dB/V  */
      rfgain = (rfagc - 9) * 3.8;
	}
	else if ((Mode_Sel & 3) == 0)
	{   /* if band = VHF  */
		if (rfagc < 9)
			rfagc = 9;
		if (rfagc > 19)
			rfagc = 19;
      /*   ct gain of the tuner */
      ctgain = -1.5 - lna_status * 22;
      /*  slope = 38dB/V  */
      rfgain = (rfagc - 9) * 3.8;
	}
	else
	{         /* band = LBand  */
		if (rfagc < 8)
			rfagc = 8;
		if (rfagc > 18)
			rfagc = 18;
      /*   ct gain of the tuner */
      ctgain = 2 - lna_status * 24;
      /*  slope = 38dB/V  */
      rfgain = (rfagc - 8) * 3.8;
	}
	/* approximate the bb_vgc curve */
   /*  slope = 23 dB/V  */
   bbgain = bbagc * 2.3;
   det_th = (PLL_Conf & 0x1c) >> 2;
   det_gain_r = (FM_Filt_RSSI & 0x18) >> 3;
   if (det_gain_r == 1)
      det_gain = 3.7;
   if (det_gain_r == 2)
		det_gain = 6;
   if (det_gain_r == 3)
      det_gain = 16;

	if ((Mode_Sel & 3) == 0)   /* if band = VHF  */
      return (int)(-rfgain - bbgain + det_th * 1.45 - det_gain - ctgain) + (Frequency -200)/30;
	else
      return (int)(-rfgain - bbgain + det_th * 1.45 - det_gain - ctgain);
}



static int Max2172ReadTemperature(void)
{
	/* Returns the IC internal temperature in degrees Celcius */
	/* Assumes ADE and ADL bits low in Reg MAX2172_ROM_ADDR   */
	/* Which they should be from the Max2172Init() routine.   */

	int ADC;

	/* if RSSI_EN bit is not 0, make it 0  */
	if ((Frac2 & 0x40) > 1)
	{
		writeAReg(MAX2172_FRAC2, (Frac2 & 0xbf));   /* RSSI_EN = 0 or manual mode */
		/* Must Send Frac0 to latch in NDiv, Frac2 or Frac1 registers: */
		writeAReg(MAX2172_FRAC0, Frac0);
	}

	/* Select Temperature RSSI path: */
	writeAReg(MAX2172_FM_FILT_RSSI, ((FM_Filt_RSSI & 0xfc) | 1));

	ADC = Max2172ReadRSSI() & 0x1f;

	/* If Frac2 initially had bit RSSI_EN = 1, return it to 1: */
	if ((Frac2 & 0x40) > 1)
	{
		writeAReg(MAX2172_FRAC2, Frac2);
		writeAReg(MAX2172_FRAC0, Frac0);
	}
	/* return FM filter and RSSI register to it's state prior to this function */
	writeAReg(MAX2172_FM_FILT_RSSI, FM_Filt_RSSI);

	return (int)(105 - (5.68 * ADC));
}
#endif	/* USE_FP_MATH */

#define WRITE_REG_TRANSFER_SIZE	2
#define READ_REG_TRANSFER_SIZE	1
static unsigned char messageBuffer[WRITE_REG_TRANSFER_SIZE];

/* readAReg will read and return the value in register 'reg' */

static int readAReg(int reg)
{
	messageBuffer[0] = (unsigned char)reg;

	MAX_writeMessage(messageBuffer, READ_REG_TRANSFER_SIZE);

	MAX_readMessage(messageBuffer, READ_REG_TRANSFER_SIZE);

	return messageBuffer[0];
}

/* writeAReg will write to register 'reg' the value 'value' */

static void writeAReg(int reg,int value)
{
    /* Note, by writing directly using this function, you do not
       update the register variables.  Thus you may corrupt the
       higher level functions such as SetLO since they are expecting
       the variables to be a true representation of the register contents.
    */

	messageBuffer[0] = (unsigned char)reg;
	messageBuffer[1] = (unsigned char)value;

	MAX_writeMessage(messageBuffer, WRITE_REG_TRANSFER_SIZE);
}



