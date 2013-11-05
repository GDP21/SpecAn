/*!
******************************************************************************
 @file   : pllgen.c

 @brief

 @date   1/4/2011

         <b>Copyright 2011 by Imagination Technologies Limited.</b>\n
         All rights reserved.  No part of this software, either
         material or conceptual may be copied or distributed,
         transmitted, transcribed, stored in a retrieval system
         or translated into any human or computer language in any
         form by any means, electronic, mechanical, manual or
         other-wise, or disclosed to third parties without the
         express written permission of Imagination Technologies
         Limited, Home Park Estate, King's Langley, Hertfordshire,
         WD4 8LZ, U.K.

 <b>Description:</b>\n

         Tool for generating SYS PLL values for Saturn.

******************************************************************************/
/*
******************************************************************************
 Modifications :-

 $Log: pllgen.c,v $

  --- Revision Logs Removed --- 




******************************************************************************/

#include "stdio.h"
#include "img_defs.h"
#include "string.h"
#include "stdlib.h"
#include "ctype.h"
#include "io.h"
#include "conio.h"
//#include "sys_config.h"


/*!
******************************************************************************
******************************************************************************/

static  IMG_CHAR *			gpszExeName = "PllGen";
static  IMG_CHAR 			gpcInputFreqCode = 'Z';
static	IMG_UINT32			ui32XTALFreqIndex = 0;
static  IMG_UINT32			ui32SourceFreq_fp = 0;
static  IMG_UINT32			ui32TargetFreq_fp = 0;
static	IMG_UINT32			ui32TargetFreq 	  = 0xFFFFFFFF;
static 	IMG_UINT32			ui32CTL0 		  = 0;
static 	IMG_UINT32			ui32CTL1 		  = 0;
static  IMG_BOOL			bAnalysisMode     = 0;

typedef struct _PllFields
{
	IMG_UINT32 ui32CLKF;
	IMG_UINT32 ui32OD;
	IMG_UINT32 ui32CLKR;
}PllFields;

static	IMG_UINT32		g_aui32XTALFrequencies_fp[10] =
{
	0x010624DD,		// 16.384
	0x01333333,		// 19.2
	0x01800000,		// 24
	0x0189374B,		// 24.576
	0x01A00000,		// 26
	0x02400000,		// 36
	0x024DD2F1,		// 36.864
	0x02666666,		// 38.4
	0x02800000,		// 40
	0x03000000		// 48
};

#define FP_SCALE   20
#define CLKF_SHIFT 4
#define CLKF_MASK  0x0001FFF0
#define CLKR_MASK  0x0000003F
#define CLKOD_MASK 0x00000007

#define EPSILON	(0xCCCC)// <-- Equivalent to 50kHz in 12.20 notation






/*!
******************************************************************************

 @Function	 ExtractFieldsFromPllRegs

******************************************************************************/

IMG_VOID ExtractFieldsFromPllRegs(IMG_UINT32 ui32CTL0, IMG_UINT32 ui32CTL1, PllFields * sPllFields)
{
	sPllFields->ui32CLKF = (ui32CTL0 & CLKF_MASK) >> CLKF_SHIFT;
	sPllFields->ui32CLKR = ui32CTL1 & CLKR_MASK;
	sPllFields->ui32OD 	 = ui32CTL0 & CLKOD_MASK;
}

/*!
******************************************************************************

 @Function	 calculate_actual_frequency_12_20

******************************************************************************/
IMG_UINT32 calculate_actual_frequency_12_20(IMG_UINT32 ui32CLKF, IMG_UINT32 ui32OD, IMG_UINT32 ui32CLKR, IMG_UINT32 ui32Xtal_fp)
{
	IMG_UINT32 ui32ResultFreq;
	IMG_UINT64 ui64InterimValue;

	// The equation for the PLL output frequency is: Fout = ( Fref / (clkR + 1) ) * ( ( clkf / 2 + 0.5 ) / (clkOD + 1) ) / PostDiv

	// Calculate the actual frequency - get the multiplications done first to keep accuracy.
	ui64InterimValue  = (IMG_UINT64)ui32Xtal_fp * (ui32CLKF + 1); // Instead of CLKF/2 + 0.5 we are using CLKF+1, so need to divide by 2 later
	ui64InterimValue /= (ui32CLKR+1);
	ui64InterimValue /= (ui32OD + 1);
	ui32ResultFreq    = (IMG_UINT32)(ui64InterimValue >> 1); // Now dividing by 2 as per the above comment.

	return ui32ResultFreq;
}

/*!
******************************************************************************

 @Function	 DisplayFrequencyFromSuppliedSettings

******************************************************************************/
IMG_VOID DisplayFrequencyFromSuppliedSettings(IMG_VOID)
{
	IMG_UINT32  ui32Freq_fp;
	PllFields	sPllFields;

	ExtractFieldsFromPllRegs(ui32CTL0, ui32CTL1, &sPllFields);
	ui32Freq_fp = calculate_actual_frequency_12_20(sPllFields.ui32CLKF, sPllFields.ui32OD, sPllFields.ui32CLKR, ui32SourceFreq_fp);

	printf("\nSupplied Values:    CLKF:%d CLKOD:%d CLKR:%d\n",
		sPllFields.ui32CLKF,
		sPllFields.ui32OD,
		sPllFields.ui32CLKR);

	{
		float		fFreq;

		fFreq = (float)(ui32Freq_fp) / (1024.0f * 1024.0f);
		printf("Frequency Produced: %.3f MHz\n", fFreq );
	}
}

/*!
******************************************************************************

 @Function	 SYS_getPLLSetupValues

******************************************************************************/
IMG_VOID SYS_getPLLSetupValues( IMG_UINT32		ui32SourceFreq_fp,
								IMG_UINT32		ui32TargetFreq_fp,
								IMG_UINT16	*	pui16CLKF,
								IMG_UINT8	*	pui8CLKR,
								IMG_UINT8	*	pui8CLKOD,
								IMG_UINT16	*	pui16BWAdj,
								IMG_UINT32	*	pui32ActualFreq_fp )
{

	IMG_UINT64	ui64CLKF_63_1;
	IMG_UINT32	ui32CLKF;
	IMG_UINT32	ui32frequencyProduced 	= 0;
	IMG_BOOL	bOutOfBounds 			= IMG_FALSE;

	IMG_UINT8	ui8ODMax 				= 8;
	IMG_UINT8	r, od;

	// Check if we're going to be setting up the PLL out of bounds.
	if( ui32TargetFreq_fp > (600 << FP_SCALE ) )
	{
		bOutOfBounds = IMG_TRUE;
		ui8ODMax = 0;
	}
	else
	{
		// Not going out of bounds, but limit searches to values of OD that don't obviously put the intermediate freq above the 600MHz Max.
		ui8ODMax = ((600 << FP_SCALE )/ui32TargetFreq_fp) - 1;
		if ( ui8ODMax > 8 )
		{
			ui8ODMax = 8;
		}
	}

	//Loop through possible values of OD
	for ( od = ui8ODMax; od < 0xFF ; od--)  // To minimise jitter we need to have OD as large as possible
	{
		// Loop through possible values of clkr
		for ( r = 0; r < 64; r++ )
		{
			// Calculate clkF in 63/1 so we can round up if necessary
			//                  clkF = (( 2 * (OD+1) * PLL 				 * (CR+1) ) / XT) - 1
			ui64CLKF_63_1 = (IMG_UINT64)  2 * (od+1) * ui32TargetFreq_fp * (r+1) * 2; // Doing all multiplications first (*2 to shift into 63/1)
			ui64CLKF_63_1 /= ui32SourceFreq_fp;										  // we can divide by this directly now rather than using XT/2 as we are in 63_1 notation
			ui64CLKF_63_1 -= 2;														  // -2 rather than -1 as we are working in 63/1 format

			// round up if the fractional bit is set - we want the closest value of CLKF possible.
			ui32CLKF = (ui64CLKF_63_1 & 1) ? ((IMG_UINT32)(ui64CLKF_63_1 >> 1)) + 1 :
			                                  (IMG_UINT32)(ui64CLKF_63_1 >> 1);

			// Verify produced frequency is within precision required
			ui32frequencyProduced = calculate_actual_frequency_12_20(ui32CLKF, od, r, ui32SourceFreq_fp);

			if( abs( ui32frequencyProduced - ui32TargetFreq_fp) < EPSILON)
			{
				// Potentially found a match - check no constraints violated.
				if((ui32CLKF << 4) & ~CLKF_MASK)
				{
					continue; // CLKF value too large
				}

				// Verify intermediate frequencies are acceptable (But only if we are drving the PLL within bounds)
				if ( !bOutOfBounds )
				{
					if((ui32frequencyProduced*(od+1)) > (600 << FP_SCALE) ||
					   (ui32frequencyProduced*(od+1)) < (120 << FP_SCALE))
					{
						continue; // Internal frequencies are out of range
					}
				}

				// Good Settings found
				*pui16CLKF			= (IMG_UINT16)ui32CLKF;
				*pui8CLKR			= (IMG_UINT8)r;
				*pui8CLKOD			= (IMG_UINT8)od;
				*pui32ActualFreq_fp	= ui32frequencyProduced;

				if(ui32CLKF)
				{
					*pui16BWAdj = (IMG_UINT16)(((2 * ui32CLKF) - 1) >> 2);
				}
				else
				{
					*pui16BWAdj = 0;
				}

				return; // Early exit!
			}
		}
	}
}


/******************************************************************************

Header printer

******************************************************************************/
void DisplayHeader( void )
{
      printf("\n");
      printf("===========================================================================\n");
      printf("=                                                                         =\n");
      printf("=                    Saturn PLL Application                               =\n");
      printf("=               (Build date %s, time %s)                   =\n", __DATE__ , __TIME__);
      printf("=                                                                         =\n");
      printf("===========================================================================\n");
      printf("\n");
}



/*!
******************************************************************************

 @Function				OutputUsage

******************************************************************************/
static IMG_VOID OutputUsage(IMG_VOID)
{
	printf("\nUsage:\n");
	printf("\nTo Calculate PLL settings from a given XTAL and desired output frequency:\n");
	printf("-------------------------------------------------------------------------\n");
    printf("%s -x <input XTAL code> -t <target PLL otuput frequency>\n\n", gpszExeName);
    printf("\nTo determine the PLL speed from existing settings:\n");
    printf("--------------------------------------------------\n");
    printf("%s -x <input XTAL code> -r <CR_TOP_SYSPLL_CTL0 value> <CR_TOP_SYSPLL_CTL1 value>\n\n", gpszExeName);
}

/*!
******************************************************************************

 @Function				OutputHelp

******************************************************************************/
static IMG_VOID OutputHelp(IMG_VOID)
{
	OutputUsage();

	printf("Options:\n");

	printf("-x <input XTAL code>                Input XTAL code (A = 16.384 MHz\n");
	printf("                                                     B = 19.2   MHz\n");
	printf("                                                     C = 24     MHz\n");
	printf("                                                     D = 24.576 MHz\n");
	printf("                                                     E = 26     MHz\n");
	printf("                                                     F = 36     MHz\n");
	printf("                                                     G = 36.864 MHz\n");
	printf("                                                     H = 38.4   MHz\n");
	printf("                                                     I = 40     MHz\n");
	printf("                                                     J = 48     MHz\n");
	printf("\n");
	printf("-t <target PLL output frequency>    Output frequency in MHz eg 200\n");
	printf("\n");
	printf("-r <CR_TOP_SYSPLL_CTL0 value> <CR_TOP_SYSPLL_CTL1 value>\n");
	printf("\n");
	printf("-h                                  Display option help\n");
	printf("\n");
}

/*!
******************************************************************************

 @Function				ValidArguments

******************************************************************************/
static IMG_BOOL ValidArguments(
	int					argc,
	char *				argv[]
)
{
	int			    i;
	IMG_CHAR		option;

	/* Loop over arguments...*/
	i = 1;
	while (i < argc)
	{
		/* If this is not an option...*/
		if ( (strlen(argv[i]) != 2) && (argv[i][0] != '-') )
		{
			/* Invalid arguments */
			return IMG_FALSE;
		}

		/* Check option...*/
		option = argv[i][1];
		switch (argv[i][1])
		{
		case 'h':
            OutputHelp();
            exit(0);

		case 'x':
			if ( gpcInputFreqCode == 'Z' )
			{
			}
			else
			{
				/* Conflicting arguments */
				printf("ERROR: %s - Conflicting arguments\n", gpszExeName);
				return IMG_FALSE;
			}

			/* Move to next argument */
			i++;

			/* Check for address */
			if (
					(i			>= argc) ||
					(argv[i][0] == '-')
				)
			{
				printf("ERROR: %s - Target frequency missing\n", gpszExeName);
				return IMG_FALSE;
			}

			/* Get input frequency code */
			sscanf(argv[i], "%c", &gpcInputFreqCode );

			/* Check and convert to internal form */
			if ( (gpcInputFreqCode >= 'A') && (gpcInputFreqCode <= 'J') )
			{
				// Okay
				ui32XTALFreqIndex = gpcInputFreqCode - 'A';
			}
			else
			{
				if ( (gpcInputFreqCode >= 'a') && (gpcInputFreqCode <= 'j') )
				{
					// Okay
					ui32XTALFreqIndex = gpcInputFreqCode - 'a';
				}
				else
				{
					printf("ERROR: %s - Invalid input XTAL frequency code\n", gpszExeName);
					return IMG_FALSE;
				}
			}

			ui32SourceFreq_fp = g_aui32XTALFrequencies_fp[ui32XTALFreqIndex];

			/* Move to next argument */
			i++;

			/* Go look at the next option */
			continue;

        case 't':
			if (ui32TargetFreq == 0xFFFFFFFF)
			{
			}
			else
			{
				/* Conflicting arguments */
				printf("ERROR: %s - Conflicting arguments\n", gpszExeName);
				return IMG_FALSE;
			}

			/* Move to next argument */
			i++;

			/* Check for address */
			if (
					(i			>= argc) ||
					(argv[i][0] == '-')
				)
			{
				printf("ERROR: %s - Target frequency missing\n", gpszExeName);
				return IMG_FALSE;
			}

			/* Get target frequency (in MHz) */
			sscanf(argv[i], "%d", &ui32TargetFreq );

			/* Check and convert to internal form */
			if ( ui32TargetFreq <= 1000 )
			{
				// Okay
			}
			else
			{
				printf("ERROR: %s - Invalid target frequency\n", gpszExeName);
				return IMG_FALSE;
			}

			ui32TargetFreq_fp = ui32TargetFreq << 20;

			/* Move to next argument */
			i++;

			/* Go look at the next option */
			continue;

			// This option allows the user to determine what frequency will be
			// generated from an existing set of PLL register settings.
			case 'r':
			if (ui32TargetFreq == 0xFFFFFFFF)
			{
				bAnalysisMode = 1;
			}
			else
			{
				/* Conflicting arguments */
				printf("ERROR: %s - Conflicting arguments\n", gpszExeName);
				return IMG_FALSE;
			}

			/* Move to next argument */
			i++;

			/* Check for address */
			if ( (i >= argc) || (argv[i][0] == '-') )
			{
				printf("ERROR: %s - CR_TOP_SYSPLL_CTL0 Value Missing\n", gpszExeName);
				return IMG_FALSE;
			}

			/* Get CR_TOP_SYSPLL_CTL0 */
			sscanf(argv[i], "%x", &ui32CTL0 );

			/* Move to next argument */
			i++;

			/* Check for address */
			if ( (i >= argc) || (argv[i][0] == '-') )
			{
				printf("ERROR: %s - CR_TOP_SYSPLL_CTL1 Value Missing\n", gpszExeName);
				return IMG_FALSE;
			}

			/* Get CR_TOP_SYSPLL_CTL0 */
			sscanf(argv[i], "%x", &ui32CTL1 );

			/* Move to next argument */
			i++;

			/* Go look at the next option */
			continue;

		default:
			/* Argument not recognised */
			printf("ERROR: %s - \"%s\" Argument not recognised\n", gpszExeName, argv[i]);
			return IMG_FALSE;
		}

		/* Move to next argument */
		i++;
	}

	/* Check we have the arguments we need...*/
	if(bAnalysisMode)
	{
		if (ui32SourceFreq_fp == 0)
		{
			printf("ERROR: %s - Input XTAL frequency code not specified\n", gpszExeName);
			return IMG_FALSE;
		}
	}
	else
	{
		if (ui32SourceFreq_fp == 0)
		{
				printf("ERROR: %s - Input XTAL frequency code not specified\n", gpszExeName);
				return IMG_FALSE;
		}

		if (ui32TargetFreq_fp == 0)
		{
				printf("ERROR: %s - Target frequency not specified\n", gpszExeName);
				return IMG_FALSE;
		}

		{
			float		fFreq;

			fFreq = (float)(ui32SourceFreq_fp) / (1024.0f * 1024.0f);
			printf("Input frequency  :         %.3f MHz\n", fFreq );
			printf("\n");
		}
	}
	/* Return success */
	return IMG_TRUE;
}


/*!
******************************************************************************

 @Function				GeneratePLLSetup

******************************************************************************/
static IMG_VOID GeneratePLLSetup( IMG_VOID )
{

	IMG_UINT32				ui32ActualTargetFreq_fp;
	IMG_UINT16				ui16ClkF;
	IMG_UINT16				ui16BWAdj;
	IMG_UINT8				ui8ClkR;
	IMG_UINT8				ui8ClkOD;

	IMG_UINT32				ui32PLLCtl0;
	IMG_UINT32				ui32PLLCtl1;

	SYS_getPLLSetupValues(	ui32SourceFreq_fp,
							ui32TargetFreq_fp,
							&ui16ClkF, &ui8ClkR, &ui8ClkOD, &ui16BWAdj,
							&ui32ActualTargetFreq_fp  );

	{
		float		fFreq;

		fFreq = (float)(ui32ActualTargetFreq_fp) / (1024.0f * 1024.0f);
		printf("Actual frequency :         %.3f MHz\n", fFreq );
		printf("\n");
	}


	ui32PLLCtl0 = (((IMG_UINT32)ui16BWAdj) << 20) | (((IMG_UINT32)ui16ClkF) << 4) | (((IMG_UINT32)ui8ClkOD) << 0);
	ui32PLLCtl1 = (((IMG_UINT32)ui8ClkR) << 0);
//	ui32SysClkDiv = ????

	printf("0x02015950 CR_TOP_SYSPLL_CTL0[31..0]: 0x%08lX\n", ui32PLLCtl0 );
	printf("0x02015954 CR_TOP_SYSPLL_CTL1[5..0] : 0x%08lX\n", ui32PLLCtl1 );
//	printf("CR_TOP_SYSCLK_DIV        : 0x%08lX\n", ui32SysClkDiv );

}


/*!
******************************************************************************

 @Function				main

******************************************************************************/
int main(
	int					argc,
	char *			    argv[]
)
{
	DisplayHeader();
	printf("%s - Copyright Imagination Technologies (c) 2011\n", gpszExeName);

    /* Validate arguments...*/
    if (!ValidArguments(argc, argv))
    {
        //OutputUsage();
        OutputHelp();
        exit(-1);
    }


    if (!bAnalysisMode)
    {
		// Generate the PLL settings based on the constraints supplied by the user
		GeneratePLLSetup();
	}
	else
	{
		// Display the actual frequency generated by the settings provided by the user
		DisplayFrequencyFromSuppliedSettings();
	}

	return 0;
}

