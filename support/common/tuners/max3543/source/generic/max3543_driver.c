/*
** FILE NAME:   $RCSfile: max3543_driver.c,v $
**
** TITLE:       MAX3543 DTV tuner driver
**
** AUTHOR:      Imagination Technologies
**
** DESCRIPTION: Implementation of a Maxim MAX3543 tuner driver
**
** NOTICE:      Copyright (C) Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/


/*
	Derived from example driver code from Maxim:
-------------------------------------------------------------------------
| MAX3543 Tuner Driver
| Author: Paul Nichol
|
| Version: 1.0.5
| Date:    6/1/10
|
|
| Copyright (C) 2010 Maxim Integrated Products.
| PLEASE READ THE ATTACHED LICENSE CAREFULLY BEFORE USING THIS SOFTWARE.
| BY USING THE SOFTWARE OF MAXIM INTEGRATED PRODUCTS, INC, YOU ARE AGREEING
| TO BE BOUND BY THE TERMS OF THE ATTACHED LICENSE, WHICH INCLUDES THE SOFTWARE
| LICENSE AND SOFTWARE WARRANTY DISCLAIMER, EVEN WITHOUT YOUR SIGNATURE.
| IF YOU DO NOT AGREE TO THE TERMS OF THIS AGREEMENT, DO NOT USE THIS SOFTWARE.
|
| IMPORTANT: This code is operate the Max3543 Multi-Band Terrestrial
| Hybrid Tuner.  Routines include: initializing, tuning, reading the
| ROM table, reading the lock detect status and tuning the tracking
| filter.  Only integer math is used in this program and no floating point
| variables are used.  Your MCU must be capable of processing unsigned 32 bit integer
| math to use this routine.  (That is: two 16 bit numbers multiplied resulting in a
| 32 bit result, or a 32 bit number divided by a 16 bit number).
|
| Revision 1.0.3 12/09/09
|   Description:
|      Various bug fixes related to type casting.  Added more modes to Max3543_Standard function
|      for all the various video and output standards.
|      Fixed bug where CFSET was read from ROM programmed to the registers, then overwritten
|      with the default register values later.
|
| Revision 1.0.4 12/15/09
|   Description:
|      Changed R Divider in Max3543_SetFrequency function to always change as a function
|      of frequency.  In the previous revision, this only happened in DTV mode.
|
| Revision 1.0.5 6/1/10
|   Description:
|      ADDED ATSC Mode to subroutine:
|      void MAX3543_Standard(standard bcstd, outputmode outmd)
|
|--------------------------------------------------------------------------
*/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#ifdef METAG
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "max3543_driver.h"
#include "max_port.h"

static UINT_16 TFRomCoefs[3][4];
static UINT_32 denominator;
static UINT_32 fracscale;
static UINT_16 regs[22];
static UINT_16 IF_Frequency;

static standard broadcast_standard;

/* table of fixed coefficients for the tracking filter equations. */

static UINT_16 co[6][5]={{ 26, 6,  68, 20, 45 },  /* VHF LO TFS */
                        { 16, 8,  88, 40, 0 },  /* VHF LO TFP */
                        { 27, 10, 54, 30,20 },  /* VHF HI TFS */
                        { 18, 10, 41, 20, 0 },  /* VHF HI TFP */
                        { 32, 10, 34, 8, 10 },  /* UHF TFS */
                        { 13, 15, 21, 16, 0 }}; /* UHF TFP */



UINT_16 MAX3543_Init(UINT_32 RfFreq)
{
	/*
		Initialize every register. Don't rely on MAX3543 power on reset.
	   Call before using the other routines in this file.
   */
   UINT_16 RegNumber;


   /*Initialize the registers in the MAX3543:*/



	regs[REG3543_VCO]			=	0x4c;
	regs[REG3543_NDIV]			=	0x2B;
	regs[REG3543_FRAC2]			=	0x8E;
	regs[REG3543_FRAC1]			=	0x26;
	regs[REG3543_FRAC0]			=	0x66;
	regs[REG3543_MODE]			=	0xd8;
	regs[REG3543_TFS]			=	0x00;
	regs[REG3543_TFP]			= 	0x00;
	regs[REG3543_SHDN]			=	0x00;
	regs[REG3543_REF]			=	0x0a;   /* XODIV bit<0>=0=divide by 4, bit<0>=1=divide by 1. */
	regs[REG3543_VAS]			=	0x17;
	regs[REG3543_PD_CFG1]		=	0x43;
	regs[REG3543_PD_CFG2]		=	0x01;
	regs[REG3543_FILT_CF]		=	0x25;
	regs[REG3543_ROM_ADDR]		=	0x00;
	regs[REG3543_IRHR]			=	0x80;
	regs[REG3543_BIAS_ADJ]		=	0x57;
	regs[REG3543_TEST1]			=	0x40;
	regs[REG3543_ROM_WRITE]		=	0x00;



   /* Write each register out to the MAX3543: */
	for (RegNumber=0;RegNumber<=MAX3543_NUMREGS;RegNumber++)
      Max3543_Write(RegNumber, regs[RegNumber]);

	/* First read calibration constants from MAX3543 and store in global
      variables for use when setting RF tracking filter */
	MAX3543_ReadROM();

   /* Define the IF frequency used.
		If using non-floating point math, enter the IF Frequency multiplied
	   by the factor LOSCALE here as an integer.
		i.e. IF_Frequency = 1445;
		If using floating point math, use the scalefrq macro:
		i.e. IF_Frequency = scalefrq(36.125);
	*/
	IF_Frequency = (UINT_16)(36.15 * LOSCALE);


	/* Calculate the denominator just once since it is dependant on the xtal freq only.
	   The denominator is used to calculate the N+FractionalN ratio.
	*/
	denominator = XTALREF * 4 * LOSCALE;

   /* The fracscale is used to calculate the fractional remainder of the N+FractionalN ratio.  */
	fracscale = 2147483648UL/denominator;

	MAX3543_Standard(DVB_T, IFOUT1_DIFF_DTVOUT);
	MAX3543_ChannelBW(BW8MHZ);
	MAX3543_SetFrequency(RfFreq);

	return TRUE;
}

/* Set the channel bandwidth.  Call with arguments: BW7MHZ or BW8MHZ */
void MAX3543_ChannelBW(UINT_16 bw)
{
   regs[REG3543_MODE] = (regs[REG3543_MODE] & 0xef) | (bw<<4);
	Max3543_Write(REG3543_MODE, regs[REG3543_MODE]);

}

/*
	Set the broadcast standared and RF signal path.
	This routine must be called prior to tuning (Set_Frequency() )
	such as in MAX3543_Init() or when necessary to change modes.

	This sub routine has 2 input/function
	1. bcstd:it set MAX3543 to optimized power detector/bias setting for each standard (dvb-t,pal…), currently it has 4 choice:
      1.1 bcstd=DVBT, optimized for DVB-T
      1.2 bcstd=DVBC, optimized for DVB-C
      1.3 bcstd=ATV1, optimized for PAL/SECAM - B/G/D/K/I
      1.4 bcstd=ATV2, optimized for SECAM-L
		1.5 bcstd=ATSC, optimized for ATSC
	2. outputmode: this setting has to match you hardware signal path, it has 3 choice:
      2.1 outputmode=IFOUT1_DIFF_IFOUT_DIFF
            signal path: IFOUT1 (pin6/7) driving a diff input IF filter (ie LC filter or 6966 SAW),
            then go back to IFVGA input (pin 10/11) and IF output of MAX3543 is pin 15/16.
            this is common seting for all DTV_only demod and hybrid demod
      2.2 outputmode=IFOUT1_SE_IFOUT_DIFF
            signal path: IFOUT1 (pin6) driving a single-ended input IF filter (ie 7251 SAW)
            then go back to IFVGA input (pin 10/11) and IF output of MAX3543 is pin 15/16.
            this is common seting for all DTV_only demod and hybrid demod
      2.3 outputmode=IFOUT2
            signal path: IFOUT2 (pin14) is MAX3543 IF output, normally it drives a ATV demod.
            The IFVGA is shutoff
            this is common setting for separate ATV demod app
*/

void MAX3543_Standard(standard bcstd, outputmode outmd)
{
	char IFSEL;
	char LNA2G;
	char SDIVG;
	char WPDA;
	char NPDA;
	char RFIFD;
	char MIXGM;
	char LNA2B;
	char MIXB;
	char IFVGAB;


	broadcast_standard = bcstd;   /* used later in tuning */


	switch ( bcstd )
      {
         case ATSC:
				LNA2G = 1;
				WPDA = 6;
				NPDA = 1;
				RFIFD = 1;
				MIXGM = 1;
				LNA2B = 1;
				MIXB = 1;
				IFVGAB = 0;
            break;
			case DVB_T:
				LNA2G = 1;
				WPDA = 4;
				NPDA = 3;
				RFIFD = 1;
				MIXGM = 1;
				LNA2B = 1;
				MIXB = 1;
				IFVGAB = 1;
				break;
			case DVB_C:
				LNA2G = 1;
				WPDA = 3;
				NPDA = 3;
				RFIFD = 1;
				MIXGM = 1;
				LNA2B = 1;
				MIXB = 1;
				IFVGAB = 1;
            break;
			case ATV:
				LNA2G = 0;
				WPDA = 3;
				NPDA = 5;
				RFIFD = 2;
				MIXGM = 0;
				LNA2B = 3;
				MIXB = 3;
				IFVGAB = 1;
            break;
			case ATV_SECAM_L:
				LNA2G = 0;
				WPDA = 3;
				NPDA = 3;
				RFIFD = 2;
				MIXGM = 0;
				LNA2B = 3;
				MIXB = 3;
				IFVGAB = 1;
            break;
			default:
				return;
		}

	   /* the outmd must be set after the standard mode bits are set.
		   Please do not change order.  */
		switch ( outmd )
      {
			case IFOUT1_DIFF_DTVOUT:
				IFSEL = 0;
				SDIVG = 0;
				break;
			case IFOUT1_SE_DTVOUT:
				IFSEL = 1;
				SDIVG = 0;
				break;
			case IFOUT2:
				IFSEL = 2;
				SDIVG = 1;
				NPDA = 3;
				LNA2G = 1;   /* overrites value chosen above for this case */
				RFIFD = 3;   /* overrites value chosen above for this case */
				break;
			default:
				return;
		}


	/* Mask in each set of bits into the register variables */
   regs[REG3543_MODE] = (regs[REG3543_MODE] & 0x7c) | IFSEL | (LNA2G<<7);
   regs[REG3543_SHDN] = (regs[REG3543_SHDN] & 0xf7) | (SDIVG<<3);
   regs[REG3543_PD_CFG1] = (regs[REG3543_PD_CFG1] & 0x88) | (WPDA<<4) | NPDA;
   regs[REG3543_PD_CFG2] = (regs[REG3543_PD_CFG2] & 0xfc) | RFIFD;
   regs[REG3543_BIAS_ADJ] = (regs[REG3543_BIAS_ADJ] & 0x82) | (MIXGM<<6) | (LNA2B<<4) | (MIXB<<2) | IFVGAB;

	/* Send each register variable: */
   Max3543_Write(REG3543_MODE, regs[REG3543_MODE]);
   Max3543_Write(REG3543_SHDN, regs[REG3543_SHDN]);
   Max3543_Write(REG3543_PD_CFG1, regs[REG3543_PD_CFG1]);
   Max3543_Write(REG3543_PD_CFG2, regs[REG3543_PD_CFG2]);
   Max3543_Write(REG3543_BIAS_ADJ, regs[REG3543_BIAS_ADJ]);

}


/*This will set the RF Frequency and all other tuning related register bits.
*/
void MAX3543_SetFrequency(UINT_32 RF_Frequency_Hz)
{
	UINT_16 RDiv, NewR, NDiv, Vdiv;
	UINT_32 Num;
	UINT_16 RF_Frequency;
	UINT_16 LO_Frequency;
	UINT_32 Rem;
	UINT_32 FracN;

		/* Convert from a carrier frequency in Hz to a MHz value scaled by LOSCALE.
		** Therefore it has units of 1/LOSCALE MHz */
	RF_Frequency = (UINT_16)(((RF_Frequency_Hz / 100) * LOSCALE) / 10000);

	LO_Frequency = RF_Frequency + IF_Frequency ;

	/* Determine VCO Divider */
	if (LO_Frequency < scalefrq(138))  /* 138MHz scaled UHF band */
	{
		Vdiv = 3;  /*  divide by 32   */
	}
	else if ( LO_Frequency < scalefrq(275))                   /* VHF Band */
	{
		Vdiv = 2;  /*  divide by 16   */
	}
	else if (LO_Frequency < scalefrq(550))
	{
		Vdiv = 1;  /*  divide by 8   */
	}
	else
	{
		Vdiv = 0;  /*  divide by 4   */
	}


	/* calculate the r-divider from the RDIV bits:
	 RDIV bits   RDIV
		00				1
		01				2
		10				4
		11				8
	*/
	RDiv = 1<<((regs[REG3543_FRAC2] & 0x30) >> 4);

	/* Set the R-Divider based on the frequency if in DVB mode.
      Otherwise set the R-Divider to 2.
	   Only send RDivider if it needs to change from the current state.
	*/
	NewR = 0;
	if ((broadcast_standard == DVB_T || broadcast_standard == DVB_C || broadcast_standard == ATV || broadcast_standard == ATV_SECAM_L) )
	{
		if ((LO_Frequency <= scalefrq(275)) && (RDiv == 1))
			NewR = 2;
		else if ((LO_Frequency > scalefrq(275)) && (RDiv == 2))
			NewR = 1;
	}
	else if (RDiv == 1) /* For now this never happens, for future use.  */
		NewR = 2;

	if (NewR != 0){
		RDiv = NewR;
		regs[REG3543_FRAC2] = (regs[REG3543_FRAC2] & 0xcf) | ((NewR-1) <<4);
		Max3543_Write(REG3543_FRAC2, regs[REG3543_FRAC2]);
	}

	/* Update the VDIV bits in the VCO variable.
	   we will write this variable out to the VCO register during the MAX3543_SeedVCO routine.
		We can write over all the other bits (D<7:2>) in that register variable because they
		will be filled in: MAX3543_SeedVCO later.
	*/
	regs[REG3543_VCO] = Vdiv;

	/* now convert the Vdiv bits into the multiplier for use in the equation:
		Vdiv   Mult
			0      4
			1      8
			2      16
			3      32
	*/
	Vdiv = 1<<(Vdiv+2);


	//#ifdef HAVE_32BIT_MATH

	/* Calculate Numerator and Denominator for N+FN calculation  */
	Num = LO_Frequency * RDiv * Vdiv * XTALSCALE;


	NDiv = (UINT_16) (Num/(UINT_32)denominator);   /* Note: this is an integer division, returns 16 bit value. */

   Max3543_Write(REG3543_NDIV,NDiv);

	/* Calculate whole number remainder from division of Num by denom:
	   Returns 16 bit value.  */
	Rem = Num - (((UINT_32) NDiv) * denominator);

	/* FracN = Rem * 2^20/Denom, Scale 2^20/Denom 2048 X larger for more accuracy. */
   /* fracscale = 2^31/denom.  2048 = 2^31/2^20  */
   FracN =(Rem*fracscale)/2048;



	/* Optional - Seed the VCO to cause it to tune faster.
		(LO_Frequency/LOSCALE) * Vdiv = the unscaled VCO Frequency.
		It is unscaled to prevent overflow when it is multiplied by vdiv.
	*/
   MAX3543_SeedVCO((LO_Frequency/LOSCALE) * Vdiv);



	regs[REG3543_FRAC2] = (regs[REG3543_FRAC2] & 0xf0) | ((UINT_16)(FracN >> 16) & 0xf);
   Max3543_Write(REG3543_FRAC2, regs[REG3543_FRAC2]);
   Max3543_Write(REG3543_FRAC1,(UINT_16)(FracN >> 8) & 0xff);
   Max3543_Write(REG3543_FRAC0, (UINT_16) FracN & 0xff);

   /* Program tracking filters and other frequency dependent registers */
   MAX3543_SetTrackingFilter(RF_Frequency);

}



/*As you tune in frequency, the tracking filter needs
to be set as a function of frequency.  Stored in the ROM on the
IC are two end points for the VHFL, VHFH, and UHF frequency bands.
This routine performs the necessary function to calculate the
needed series and parallel capacitor values from these two end
points for the current frequency.  Once the value is calculated,
it is loaded into the Tracking Filter Caps register and the
internal capacitors are switched in tuning the tracking
filter.
*/
void  MAX3543_SetTrackingFilter(UINT_16 RF_Frequency)
{
	/*  Calculate the series and parallel capacitor values for the given frequency  */
	/*  band.  These values are then written to the registers.  This causes the     */
	/*  MAX3543's internal series and parallel capacitors to change thus tuning the */
	/*  tracking filter to the proper frequency.                                    */

   UINT_16 TFB, tfs, tfp, RFin, HRF;



   /*  Set the TFB Bits (Tracking Filter Band) for the given frequency. */
	if (RF_Frequency < scalefrq(196))  /* VHF Low Band */
   {
        TFB = VHF_L;
   }
   else if (RF_Frequency < scalefrq(440)) /* VHF High  196-440 MHz */
   {
        TFB = VHF_H;
   }
   else{    /* UHF */
        TFB = UHF;
	}

   /*  Set the RFIN bit.  RFIN selects a input low pass filter */
	if (RF_Frequency < scalefrq(345)){  /* 345 MHz is the change over point. */
		RFin = 0;
	}
	else{
      RFin = 1;
	}

	if (RF_Frequency < scalefrq(110)){  /* 110 MHz is the change over point. */
		HRF = 1;
	}
	else{
      HRF = 0;
	}

	/* Write the TFB<1:0> Bits and the RFIN bit into the IFOVLD register */
	/* TFB sets the tracking filter band in the chip, RFIN selects the RF input */
   regs[REG3543_MODE] = (regs[REG3543_MODE] & 0x93 ) | (TFB << 2) | (RFin << 6) | (HRF<<5);
   Max3543_Write(REG3543_MODE,(regs[REG3543_MODE])) ;

   tfs = tfs_i(TFRomCoefs[TFB][SER0], TFRomCoefs[TFB][SER1],  RF_Frequency/LOSCALE, co[TFB*2]);
   tfp = tfs_i(TFRomCoefs[TFB][PAR0],TFRomCoefs[TFB][PAR1],  RF_Frequency/LOSCALE, co[(TFB*2)+1]);

	/* Write the TFS Bits into the Tracking Filter Series Capacitor register */
	if (tfs > 255)   /* 255 = 8 bits of TFS */
		tfs = 255;
//	if (tfs < 0)
//		tfs = 0;
   regs[REG3543_TFS] = tfs;

	/* Write the TFP Bits into the Tracking Filter Parallel Capacitor register */
	if (tfp > 63)   /* 63 = 6 bits of TFP  */
		tfp = 63;
//	if (tfp < 0)
//		tfp = 0;
   regs[REG3543_TFP] = (regs[REG3543_TFP] & 0xc0 ) | tfp;

	   /*  Send registers that have been changed */
   /*  Maxim evkit I2c communication... Replace by microprocessor specific code */
   Max3543_Write(REG3543_TFS,(regs[REG3543_TFS])) ;
   Max3543_Write(REG3543_TFP,(regs[REG3543_TFP])) ;


}





/* calculate aproximation for Max3540 tracking filter useing integer math only */

UINT_16 tfs_i(UINT_16 S0, UINT_16 S1, UINT_16 FreqRF, UINT_16 c[5])
{  UINT_16 i,y,y1,y2,y3,y4,y5,y6,y7,add;
   UINT_32 res;

/* y=4*((64*c[0])+c[1]*S0)-((64*c[2]-(S1*c[3]))*(FreqRF))/250;   */
   y1=64*c[0];
   y2=c[1]*S0;
   y3=4*(y1+y2);
   y4=S1*c[3];
   y5=64*c[2];
   y6=y5-y4;
   y7=(y6*(FreqRF))/250;
   if (y7<y3)
   { y= y3-y7;
                    /* above sequence has been choosen to avoid overflows */
     y=(10*y)/111;                /* approximation for nom*10*LN(10)/256 */
     add=y; res=100+y;
     for (i=2; i<12; i++)
     {
       add=(add*y)/(i*100);       /* this only works with 32bit math */
       res+=add;
     }
   }
   else
     res=0;
   if (((UINT_32)res+50*1)>((UINT_32)100*c[4])) res=(res+50*1)/100-c[4];
   else res=0;

   if (res<255) return (UINT_16) res; else return 255;
}

/*
   As soon as you program the Frac0 register, a state machine is started to find the
	correct VCO for the N and Fractional-N values entered.
	If the VASS bit is set, the search routine will start from the band and
	sub-band that is currently programmed into the VCO register (VCO and VSUB bits = seed).
	If you seed the register with	bands close to where the auto routine will
	finally select, the search routine will finish much faster.
	This routine determines the best seed to use for the VCO and VSUB values.
	If VASS is cleared, the search will start as the lowest VCO and search up.
	This takes much longer.  Make sure VASS is set before using this routine.
	For the seed value to be read in the VCO register, it must be there prior to
	setting the Frac0 register.  So call this just before setting the Fractional-N
	registers.  The VASS bit is bit 5 of register 10 (or REG3543_VAS).
*/
void MAX3543_SeedVCO(UINT_16 Fvco){
   /* Fvco is the VCO frequency in MHz and is not scaled by LOSCALE  */
	UINT_16 VCO;
	UINT_16 VSUB = 0;
	if (Fvco  <= 2750)
	{
		/* The VCO seed: */

		VCO = 0;
		/* Determine the VCO sub-band (VSUB) seed:  */
	   if (Fvco  < 2068)
         VSUB = 0;
		else if (Fvco  < 2101)
         VSUB = 1;
		else if (Fvco  < 2137)
         VSUB = 2;
		else if (Fvco  < 2174)
         VSUB = 3;
		else if (Fvco  < 2215)
         VSUB = 4;
		else if (Fvco  < 2256)
         VSUB = 5;
		else if (Fvco  < 2300)
         VSUB = 6;
		else if (Fvco  < 2347)
         VSUB = 7;
		else if (Fvco  < 2400)
         VSUB = 8;
		else if (Fvco  < 2453)
         VSUB = 9;
		else if (Fvco  < 2510)
         VSUB = 10;
		else if (Fvco  < 2571)
         VSUB = 11;
		else if (Fvco  < 2639)
         VSUB = 12;
		else if (Fvco  < 2709)
         VSUB = 13;
		else if (Fvco  < 2787)
         VSUB = 14;
	}
	else if (Fvco  <= 3650)
	{
		/* The VCO seed: */
		VCO = 1;
		/* Determine the VCO sub-band (VSUB) seed:  */
	   if (Fvco  <= 2792)
         VSUB = 1;
		else if (Fvco  <= 2839)
         VSUB = 2;
		else if (Fvco  <= 2890)
         VSUB = 3;
		else if (Fvco  <= 2944)
         VSUB = 4;
		else if (Fvco  <= 3000)
         VSUB = 5;
		else if (Fvco  <= 3059)
         VSUB = 6;
		else if (Fvco  <= 3122)
         VSUB = 7;
		else if (Fvco  <= 3194)
         VSUB = 8;
		else if (Fvco  <= 3266)
         VSUB = 9;
		else if (Fvco  <= 3342)
         VSUB = 10;
		else if (Fvco  <= 3425)
         VSUB = 11;
		else if (Fvco  <= 3516)
         VSUB = 12;
		else if (Fvco  <= 3612)
         VSUB = 13;
		else if (Fvco  <= 3715)
         VSUB = 14;
	}
	else
	{
		/* The VCO seed: */
		VCO = 2;
		/* Determine the VCO sub-band (VSUB) seed:  */
	   if (Fvco  <= 3658)
         VSUB = 0;
		else if (Fvco  <= 3716)
         VSUB = 2;
		else if (Fvco  <= 3776)
         VSUB = 2;
		else if (Fvco  <= 3840)
         VSUB = 3;
		else if (Fvco  <= 3909)
         VSUB = 4;
		else if (Fvco  <= 3980)
         VSUB = 5;
		else if (Fvco  <= 4054)
         VSUB = 6;
		else if (Fvco  <= 4134)
         VSUB = 7;
		else if (Fvco  <= 4226)
         VSUB = 8;
		else if (Fvco  <= 4318)
         VSUB = 9;
		else if (Fvco  <= 4416)
         VSUB = 10;
		else if (Fvco  <= 4520)
         VSUB = 11;
		else if (Fvco  <= 4633)
         VSUB = 12;
		else if (Fvco  <= 4751)
         VSUB = 13;
		else if (Fvco  <= 4876)
         VSUB = 14;
		else
         VSUB = 15;
	}
	/* VCO = D<7:6>, VSUB = D<5:2> */
	regs[REG3543_VCO] = (regs[REG3543_VCO] & 3) | (VSUB<<2) | (VCO<<6);
	/* Program the VCO register with the seed: */
	Max3543_Write(REG3543_VCO, regs[REG3543_VCO]);

}



/* Returns the lock detect status.  This is accomplished by
   examining the ADC value read from the MAX3543.  The ADC
	value is the tune voltage digitized.  If it is too close to
	ground or Vcc, the part is unlocked.  The ADC ranges from 0-7.
	Values 1 to 6 are considered locked.  0 or 7 is unlocked.
	Returns True for locked, fase for unlocked.
*/
BOOL MAX3543_LockDetect(void)
{
   BOOL vcoselected;
	UINT_16 tries = 65535;
	char adc;

	/* The ADC will not be stable until the VCO is selected.
	   usually the selection process will take 25ms or less but
		it could theoretically take 100ms.  Set tries to some number
		of your processor clocks corresponding to 100ms.
		You have to take into account all instructions processed
		in determining this time.  I am picking a value out of the air
		for now.
	*/
	vcoselected = FALSE;
	while ((--tries > 0) && (vcoselected == FALSE))
	{
		if ((Max3543_Read(REG3543_VAS_STATUS) & 1) == 1)
			vcoselected = TRUE;
	}
	/* open the ADC latch:  ADL=0, ADE=1  */
	regs[REG3543_VAS] = (regs[REG3543_VAS] & 0xf3) | 4;
	Max3543_Write(REG3543_VAS, regs[REG3543_VAS]);
	/* ADC = 3 LSBs of Gen Status Register:  */
	adc = Max3543_Read(REG3543_GEN_STATUS) & 0x7;




	/* locked if ADC within range of 1-6: */
	if ((adc<1 ) || (adc>6))
		return FALSE;
	else
		return TRUE;
}



void MAX3543_ReadROM(void)
{
   /* Read the ROM table, extract tracking filter ROM coefficients,
		IRHR and CFSET constants.
		This is to be called after the Max3543 powers up.
	*/
	UINT_16 rom_data[11];
	UINT_16 i;

	for (i=0; i <= 10; i++)
	{
		Max3543_Write(REG3543_ROM_ADDR,i);   /*Select ROM Row by setting address register */
		rom_data[i] = Max3543_Read(REG3543_ROM_READ);  /* Read from ROMR Register */
	}


	/* The ROM address must be returned to 0 for normal operation or the part will not be biased properly. */
	Max3543_Write(REG3543_ROM_ADDR,0);

	/* Copy all of ROM Row 10 to Filt_CF register. */
	Max3543_Write(REG3543_FILT_CF,rom_data[10]);

	/* Copy the IRHR ROM value to the IRHR register: */
	Max3543_Write(REG3543_IRHR, rom_data[0xb]);


	/* assemble the broken up word pairs from the ROM table  into complete ROM coefficients:  */
	TFRomCoefs[VHF_L][SER0] = (rom_data[1] & 0xFC) >> 2;  /*'LS0 )*/
	TFRomCoefs[VHF_L][SER1] = ((rom_data[1] & 0x3 ) << 4) + ((rom_data[2] & 0xf0) >> 4);  /* 'LS1*/
	TFRomCoefs[VHF_L][PAR0] = ((rom_data[2] & 0xf) << 2) + ((rom_data[3] & 0xc0) >> 6);  /*'LP0*/
	TFRomCoefs[VHF_L][PAR1] = rom_data[3] & 0x3f;  /*LP1 */

	TFRomCoefs[VHF_H][SER0] = ((rom_data[4] & 0xfc) >> 2);  /*'HS0 */
	TFRomCoefs[VHF_H][SER1] = ((rom_data[4] & 0x3) << 4) + ((rom_data[5] & 0xF0) >> 4);  /*'HS1 */
	TFRomCoefs[VHF_H][PAR0] = ((rom_data[5] & 0xf) << 2) + ((rom_data[6] & 0xc0) >> 6);  /*'HP0 */
	TFRomCoefs[VHF_H][PAR1] = rom_data[6] & 0x3F;  /*'HP1 */

	TFRomCoefs[UHF][SER0] =  ((rom_data[7]  & 0xFC) >> 2);  /*'US0 */
	TFRomCoefs[UHF][SER1] = ((rom_data[7] & 0x3) << 4) + ((rom_data[8] & 0xf0) >> 4 );  /*'US1 */
	TFRomCoefs[UHF][PAR0] = ((rom_data[8] & 0xF) << 2) + ((rom_data[9] & 0xc0) >> 6);  /*'UP0 */
	TFRomCoefs[UHF][PAR1] = rom_data[9] & 0x3f;  /*'UP1 */

 }



#define WRITE_REG_TRANSFER_SIZE	2
#define READ_REG_TRANSFER_SIZE	1
static unsigned char messageBuffer[WRITE_REG_TRANSFER_SIZE];

 /* readAReg will read and return the value in register 'reg' */

	/* This reads and returns a byte from the Max3543. */
unsigned short Max3543_Read(unsigned short reg)
{
	messageBuffer[0] = (unsigned char)reg;

	MAX_writeMessage(messageBuffer, READ_REG_TRANSFER_SIZE);

	MAX_readMessage(messageBuffer, READ_REG_TRANSFER_SIZE);

	return messageBuffer[0];
}


	/* This function sends out a byte of data. */
void  Max3543_Write(unsigned short RegAddr, unsigned short data)
{
	messageBuffer[0] = (unsigned char)RegAddr;
	messageBuffer[1] = (unsigned char)data;

	MAX_writeMessage(messageBuffer, WRITE_REG_TRANSFER_SIZE);
}



