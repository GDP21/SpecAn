/*
** FILE NAME:   $RCSfile: max3543_driver.h,v $
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
------------------------------------------------------
| Max3543_Driver.h, v 1.0.5, 6/1/10, Paul Nichol
| Description: Max3543 Driver Includes.
|
| Copyright (C) 2010 Maxim Integrated Products
|
------------------------------------------------------
*/




#ifndef MAX3543_DRIVER_H
#define MAX3543_DRIVER_H

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif



typedef unsigned long       DWORD;
typedef int                 BOOL;

/* integer only mode = 1, floating point mode = 0 */
#define intmode 1


#define MAX3543_ADDR 0xc0

#define MAX3543_NUMREGS 0x15


/* Register Address offsets.  Used when sending or indexing registers.  */
#define REG3543_VCO 0
#define REG3543_NDIV 0x1
#define REG3543_FRAC2 0x2
#define REG3543_FRAC1 0x3
#define REG3543_FRAC0 0x4
#define REG3543_MODE 0x5
#define REG3543_TFS 0x6
#define REG3543_TFP 0x7
#define REG3543_SHDN 0x8
#define REG3543_REF 0x9
#define REG3543_VAS 0xa
#define REG3543_PD_CFG1 0xb
#define REG3543_PD_CFG2 0xc
#define REG3543_FILT_CF 0xd
#define REG3543_ROM_ADDR 0xe
#define REG3543_IRHR 0xf
#define REG3543_ROM_READ 0x10
#define REG3543_VAS_STATUS 0x11
#define REG3543_GEN_STATUS 0x12
#define REG3543_BIAS_ADJ 0x13
#define REG3543_TEST1 0x14
#define REG3543_ROM_WRITE 0x15

/* Band constants: */
#define VHF_L 0
#define VHF_H 1
#define UHF 2

/* Channel Bandwidth: */
#define BW7MHZ 0
#define BW8MHZ 1

#define SER0 0
#define SER1 1
#define PAR0 2
#define PAR1 3




typedef enum  {IFOUT1_DIFF_DTVOUT, IFOUT1_SE_DTVOUT,IFOUT2} outputmode;

typedef enum {DVB_T, DVB_C, ATV, ATV_SECAM_L, ATSC} standard;


/* Note:
   The SetFrequency() routine must make it's calculations without
	overflowing 32 bit accumulators.  This is a difficult balance of LO, IF and Xtal frequencies.
	Scaling factors are applied to these frequencies to keep the numbers below the 32 bit result during
	caltculations.   The calculations have been checked for only the following combinations of frequencies
	and settings: Xtal freqencies of 16.0MHz, 20.25 MHz, 20.48 MHz; IF Frequencies of 30.0 MHz and 30.15MHz;
	R-Dividers /1 and /2.  Any combination of the above numbers may be used.
	If other combinations or frequencies are needed, the scaling factors: LOSCALE and XTALSCALE must be
	recalculated.  This has been done in a spreadsheet calc.xls.  Without checking these
	scale factors carefully, there could be overflow and tuning errors or amplitude losses due to an
	incorrect tracking filter setting.
*/

/* Scaling factor for the IF and RF freqencies.
	Freqencies passed to functions must be multiplied by this factor.
	(See Note above).
*/
#define LOSCALE 40

/* Scaling factor for Xtal frequency.
   Use 32 for 16.0MHz, 25 for 20.48 and 4 for 20.25MHz.
	(See Note above).
*/
#define XTALSCALE 32

#if intmode
	/* Macros used for scaling frequency constants.  */
	/* Use this form if using floating point math. */

	#define scalefrq(x) ( (UINT_32) ( ( (UINT_16) x) * (UINT_16) LOSCALE ) )
	#define scalextal(x) ( (UINT_32) ( ( (UINT_16) x ) * (UINT_16) XTALSCALE ) )


	/* Note, this is a scaling factor for the Xtal Reference applied to the MAX3543 Xtal Pin.
		The only valid frequencies are 16.0, 20.25 or 20.48MHz and only with the following conditions:
		RDiv = /1 or RDiv = /2, IF = 36.0MHz, IF = 36.15 MHz.
		(See Note above).
	*/
	#define XTALREF (16 * XTALSCALE)
	/* 20.25 * XTALSCALE = 162, where XTALSCALE=8
		Use this form if NOT using floating point math.
	*/
#else
	/* Macros used for scaling frequency constants.  */
	/* Use this form if NOT using floating point math. */
		#define scalefrq(x)  ( (unsigned short) ( ( (float) x ) * (float) LOSCALE ) )
		#define scalextal(x) ( (unsigned short) ( ( (float) x ) * (float) XTALSCALE ) )

	/* Note, this is a scaling factor for the Xtal Reference applied to the MAX3543 Xtal Pin.
		The only valid frequencies are 16.0, 20.25 or 20.48MHz and only with the following conditions:
		RDiv = /1 or RDiv = /2, IF = 36.0MHz, IF = 36.15 MHz.
		(See Note above).
	*/
	#define XTALREF scalextal(20.25)
	/* Use this form if NOT using floating point math. */
	/* #define XTALREF 81 */
	/* (XTALSCALE * Reference frequency 20.24 * 4 = 81) */
#endif




#define ATV_SINGLE 2

typedef short INT_16;           /* compiler type for 16 bit integer */
typedef unsigned short UINT_16; /* compiler type for 16 bit unsigned integer */
typedef unsigned long UINT_32;  /* compiler type for 32 bit unsigned integer */

UINT_16 MAX3543_Init(UINT_32 RfFreq);
void MAX3543_SetFrequency(UINT_32 Frequency);
BOOL MAX3543_LockDetect(void);
void MAX3543_Standard(standard bcstd, outputmode outmd);
void MAX3543_ChannelBW(UINT_16 bw);

void MAX3543_SetTrackingFilter(UINT_16 Freq);
void MAX3543_ReadROM(void);
UINT_16 tfs_i(UINT_16 S0, UINT_16 S1, UINT_16 FreqRF, UINT_16 c[5]);
void MAX3543_SeedVCO(UINT_16 Fvco);


/******* External functions called by Max3543 code *******/


/*   The implementation of these functions is left for the end user.  */
/*   This is because of the many different microcontrollers and serial */
/*   I/O methods that can be used.  */

void Max3543_Write(unsigned short RegAddr, unsigned short data);

/*   This function sends out a byte of data using the following format.    */
/*   Start, IC_address, ACK, Register Address, Ack, Data, Ack, Stop */

/*   IC_address is 0xC0 or 0xC4 depending on JP8 of the Max3543 evkit board. */
/*   0xC0 if the ADDR pin of the Max3543 is low, x0C4 if the pin is high. */
/*   The register address is the Index into the register */
/*   you wish to fill.*/

unsigned short Max3543_Read(unsigned short reg);

/*   This reads and returns a byte from the Max3543.    */
/*   The read sequence is: */
/*   Start, IC_address, ACK, Register Address, ack, Start, DeviceReadAddress, ack, */
/*   Data, NAck, Stop */
/*   Note that there is a IC_Address (0xC0 or 0xC4 as above) and a Device Read */
/*   Address which is the IC_Address + 1  (0xC1 or 0xC5).    */
/*   There are also two start conditions in the read back sequence. */
/*   The Register Address is an index into the register you */
/*   wish to read back. */


#endif


