/*
** FILE NAME:   $RCSfile: si2153_scbm_giodrv.c,v $
**
** TITLE:       SiLabs tuner driver
**
** AUTHOR:      Imagination Technologies
**
** DESCRIPTION: Implementation of SCBM port mini driver
**
** NOTICE:      Copyright (C) Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

/* Keep these first ... */
#ifdef METAG
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "si2153_port.h"

#include "scbm_api.h"
#include "gpio_api.h"

#if SCBM_PORT_NUMBER==0
	#define RESET_GPIO_BLOCK	GPIO_PIN_SPI0_CS2_BLOCK
	#define RESET_GPIO_PIN		GPIO_PIN_SPI0_CS2_PIN
#elif SCBM_PORT_NUMBER==1
	#define RESET_GPIO_BLOCK	GPIO_PIN_TS_VALID1_BLOCK
	#define RESET_GPIO_PIN		GPIO_PIN_TS_VALID1_PIN
#elif SCBM_PORT_NUMBER==2
	#define RESET_GPIO_BLOCK	GPIO_PIN_TS_VALID2_BLOCK
	#define RESET_GPIO_PIN		GPIO_PIN_TS_VALID2_PIN
#else
	#error "SCBM_PORT_NUMBER not defined."
#endif

/* SCBM port */
//TBD Change to be multi-context...
SCBM_PORT_T SiScbmPort;
GPIO_PIN_T  resetTunerPin;

#ifndef SCBCLK_RATE_KHZ
	#error "Must define scb clock rate."
#endif

/* SCBM settings */
static SCBM_SETTINGS_T SiScbmSettings =
{
    400,    /* Data transfer bit rate (in kHz) */
    SCBCLK_RATE_KHZ,  /* Core clock (in kHz) */
    0,      /* Bus delay (in ns) */
    0		/* block index, default */
};

/* Last SCBM error */
static unsigned long scbmInitError = 0;

/*
** FUNCTION:    Si_setupPort
**
** DESCRIPTION: Sets up port
**
** INPUTS:      portNumber	serial port number to use
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int Si_setupPort(int portNumber)
{
    unsigned long status;

	(void)GPIOInit();

    GPIO_PIN_SETTINGS_T gpioSettings;
    gpioSettings.Direction = GPIO_DIR_OUTPUT;
    gpioSettings.Output.Level = GPIO_LEVEL_HIGH; /* Inactive reset */
    gpioSettings.Output.DriveStrength = GPIO_DRIVE_4MA;
    gpioSettings.Output.Slew = GPIO_SLEW_SLOW;

	GPIOAddPin(&resetTunerPin, RESET_GPIO_BLOCK, RESET_GPIO_PIN, &gpioSettings);

	switch(portNumber)
	{
			/* Valid port numbers, so set block index in settings structure to this number */
		case(0):
		case(1):
		case(2):
			SiScbmSettings.ui32BlockIndex = portNumber;
			break;
		default:
				/* Not supported port number, so return failure */
			return 0;
			break;
	}

    status = SCBMInit(&SiScbmPort,
                      &SiScbmSettings,
                      NULL,
                      0);

    if (status == SCBM_STATUS_SUCCESS)
    {
        return 1;
    }
    else
    {
        scbmInitError = status;
        return 0;
    }
}


/*
** FUNCTION:    Si_shutdownPort
**
** DESCRIPTION: Shuts down the port
**
** INPUTS:      void
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int Si_shutdownPort(void)
{
	/* Cancel any activity, then stop the peripheral driver */
	SCBMCancel(&SiScbmPort);

	SCBMDeinit(&SiScbmPort);

	GPIORemovePin(&resetTunerPin);

	GPIODeinit();

	return 1;
}





/************************************************************************************************************************
	NAME: L0_Init function
	DESCRIPTION:layer 0 initialization function
              Used to sets the layer 0 context parameters to startup values.
			  The I2C address of the Si2173 is set in the Si2153_L1_API_Init procedure.
              It is automatically called by the Layer 1 init function.
	Parameter:	Pointer to L0 (I2C) Context -
	Porting:	In most cases, no modifications should be required.
	Returns:    void
************************************************************************************************************************/
void   L0_Init  (L0_Context *pContext)
{
    (pContext)->address             = 0;

    return;
};
/************************************************************************************************************************/
/************************************************************************************************************************
  NAME: system_wait
  DESCRIPTION:	Delay for time_ms (milliseconds)
  Porting:		Replace with embedded system delay function
  Returns:		nothing
************************************************************************************************************************/
void system_wait(int time_ms)
{
	KRN_TASKQ_T queue;
	DQ_init(&queue);

	/* This assumes that the timer tick is 1ms */
	KRN_hibernate(&queue, time_ms);

	return;
}
/************************************************************************************************************************
	NAME: SendRSTb
	DESCRIPTION: Set Si2153 RSTb from low for 1 ms, then return high to reset Si2153
	Porting:    Replace with system GPIO that resets Si2153
	Parameters:  nothing
	Returns:    nothing
************************************************************************************************************************/
void SendRSTb (void)
{
	/* set RSTB low */
	GPIOSet(&resetTunerPin, GPIO_LEVEL_LOW);

	/* wait for 1 ms */
	system_wait(1);              /* wait at least 100us */

	/* set RSTB high to bring the Si2153 out of Reset mode*/
	GPIOSet(&resetTunerPin, GPIO_LEVEL_HIGH);

	return;
}
/************************************************************************************************************************
  NAME:		   L0_ReadCommandBytes function
  DESCRIPTION:Read iNbBytes from the i2c device into pucDataBuffer, return number of bytes read
  Parameter: pointer to the Layer 0 context.
  Parameter: iI2CIndex, the index of the first byte to read.
  Parameter: iNbBytes, the number of bytes to read.
  Parameter: *pbtDataBuffer, a pointer to a buffer used to store the bytes
  Porting:    Replace with embedded system I2C read function
  Returns:    Actual number of bytes read.
************************************************************************************************************************/
int L0_ReadCommandBytes(L0_Context* i2c, int iNbBytes, unsigned char *pucDataBuffer)
{
	unsigned long status;
	unsigned long nbReadBytes;

    status = SCBMRead(&SiScbmPort,//TBD Change to be multi-context...
                       i2c->address,
                       pucDataBuffer,
                       iNbBytes,
                       &nbReadBytes,
                       NULL,
                       SCBM_INF_TIMEOUT);

	if (status != SCBM_STATUS_SUCCESS)
		nbReadBytes = 0;

    return (int)nbReadBytes;
}

/************************************************************************************************************************
  NAME:  L0_WriteCommandBytes
  DESCRIPTION:  Write iNbBytes from pucDataBuffer to the i2c device, return number of bytes written
  Porting:    Replace with embedded system I2C write function
  Returns:    Number of bytes written
************************************************************************************************************************/
int L0_WriteCommandBytes(L0_Context* i2c, int iNbBytes, unsigned char *pucDataBuffer)
{
	unsigned long status;
	unsigned long nbWrittenBytes;

    status = SCBMWrite(&SiScbmPort,//TBD Change to be multi-context...
                       i2c->address,
                       pucDataBuffer,
                       iNbBytes,
                       &nbWrittenBytes,
                       NULL,
                       SCBM_INF_TIMEOUT);

    if (status != SCBM_STATUS_SUCCESS)
		nbWrittenBytes = 0;

    return nbWrittenBytes;
}

/************************************************************************************************************************/
