/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/msi_tuner/source/generic/msi_scbm_giodrv.c,v $
**
** TITLE:       MSI tuner driver
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Implementation of SCBM port mini driver
**
** NOTICE:      Copyright (C) 2009-2010, Imagination Technologies Ltd.
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
#include <MeOS.h>
#include <string.h>

#include "msi_port.h"

#include "scbm_api.h"

/* I2C address of the MSI tuner */
#define MSI_SCBM_ADDRESS (0x4c)

/* Limit the number of attempts at transfers to this */
#define MAX_TRANSFER_ATTEMPTS	2

/* SCBM port */
static SCBM_PORT_T msiScbmPort;

#ifndef SCBCLK_RATE_KHZ
#error "Must define scb clock rate."
#endif

/* SCBM settings */
static SCBM_SETTINGS_T msiScbmSettings =
{
    400,    /* Data transfer bit rate (in kHz) */
    SCBCLK_RATE_KHZ,  /* Core clock (in kHz) */
    0,      /* Bus delay (in ns) */
    0		/* block index, default */
};

/* Last SCBM error */
static unsigned long scbmLastError = 0;

/*
** FUNCTION:    MSI_setupPort
**
** DESCRIPTION: Sets up port
**
** INPUTS:      dmacChannel	DMAC channel for the serial port driver to use.
**				portNumber	serial port number to use
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int MSI_setupPort(int dmacChannel, int portNumber)
{
    unsigned long status;

    /* DMAC not used for SCBM, so ignore parameter */
    (void)dmacChannel;

	switch(portNumber)
	{
			/* Valid port numbers, so set block index in setting structure to this number */
		case(0):
		case(1):
		case(2):
			msiScbmSettings.ui32BlockIndex = portNumber;
			break;
		default:
				/* Not supported port number, so return failure */
			return 0;
			break;
	}

    status = SCBMInit(&msiScbmPort,
                      &msiScbmSettings,
                      NULL,
                      0);

    if (status == SCBM_STATUS_SUCCESS)
    {
        return 1;
    }
    else
    {
        scbmLastError = status;
        return 0;
    }
}


/*
** FUNCTION:    MSI_shutdownPort
**
** DESCRIPTION: Shuts down the port
**
** INPUTS:      void
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int MSI_shutdownPort(void)
{
	/* Cancel any activity, then stop the peripheral driver */
	SCBMCancel(&msiScbmPort);

	SCBMDeinit(&msiScbmPort);

	return 0;
}

/*
** FUNCTION:    MSI_writeMessage
**
** DESCRIPTION: Writes a message to the tuner
**
** INPUTS:      buffer  Buffer containing message to send
**              size    Number of bytes in message
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int MSI_writeMessage(unsigned char *buffer, unsigned long size)
{
    static unsigned long status, numBytesWritten, portWriteErrorStatus;	// static for debug purposes only...
	int numAttempts = 0;

	do
	{
		status = SCBMWrite(&msiScbmPort,
						   MSI_SCBM_ADDRESS,
						   buffer,
						   size,
						   &numBytesWritten,
						   NULL,
						   SCBM_INF_TIMEOUT);

		portWriteErrorStatus = SCBMGetErrorStatus(&msiScbmPort);


		if ((status == SCBM_STATUS_SUCCESS) && (numBytesWritten == size))
		{	/* successful transfer */
			return 1;
	    }

		/* The MSI002 can fail to ack its address, in that case try to send data again */
	} while(++numAttempts < MAX_TRANSFER_ATTEMPTS);

	/* Failed to send out message */
	scbmLastError = status;
	return 0;
}
