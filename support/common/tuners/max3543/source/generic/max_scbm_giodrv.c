/*
** FILE NAME:   $RCSfile: max_scbm_giodrv.c,v $
**
** TITLE:       Maxim tuner driver
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

#include "max_port.h"

#include "scbm_api.h"

/* I2C address of the Maxim tuner */
#define MAX_SCBM_ADDRESS (0xC0/2)

/* SCBM port */
static SCBM_PORT_T maxScbmPort;

#ifndef SCBCLK_RATE_KHZ
#error "Must define scb clock rate."
#endif

/* SCBM settings */
static SCBM_SETTINGS_T maxScbmSettings =
{
    400,    /* Data transfer bit rate (in kHz) */
    SCBCLK_RATE_KHZ,  /* Core clock (in kHz) */
    0,      /* Bus delay (in ns) */
    0		/* block index, default */
};

/* Last SCBM error */
static unsigned long scbmLastError = 0;

/*
** FUNCTION:    MAX_setupPort
**
** DESCRIPTION: Sets up port
**
** INPUTS:      portNumber	serial port number to use
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int MAX_setupPort(int portNumber)
{
    unsigned long status;

	switch(portNumber)
	{
			/* Valid port numbers, so set block index in settings structure to this number */
		case(0):
		case(1):
		case(2):
			maxScbmSettings.ui32BlockIndex = portNumber;
			break;
		default:
				/* Not supported port number, so return failure */
			return 0;
			break;
	}

    status = SCBMInit(&maxScbmPort,
                      &maxScbmSettings,
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
** FUNCTION:    MAX_shutdownPort
**
** DESCRIPTION: Shuts down the port
**
** INPUTS:      void
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int MAX_shutdownPort(void)
{
	/* Cancel any activity, then stop the peripheral driver */
	SCBMCancel(&maxScbmPort);

	SCBMDeinit(&maxScbmPort);

	return 1;
}

/*
** FUNCTION:    MAX_writeMessage
**
** DESCRIPTION: Writes a message to the tuner
**
** INPUTS:      buffer  Buffer containing message to send
**              size    Number of bytes in message
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int MAX_writeMessage(unsigned char *buffer, unsigned long size)
{
    static unsigned long status, numBytesWritten, portWriteErrorStatus;	// static for debug purposes only...

    status = SCBMWrite(&maxScbmPort,
                       MAX_SCBM_ADDRESS,
                       buffer,
                       size,
                       &numBytesWritten,
                       NULL,
                       SCBM_INF_TIMEOUT);

	portWriteErrorStatus = SCBMGetErrorStatus(&maxScbmPort);


    if ((status == SCBM_STATUS_SUCCESS) && (numBytesWritten == size))
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
** FUNCTION:    MAX_readMessage
**
** DESCRIPTION: Writes a message to the tuner
**
** INPUTS:      buffer  Buffer containing message to send
**              size    Number of bytes in message
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int MAX_readMessage(unsigned char *buffer, unsigned long size)
{
    static unsigned long status, numBytesRead, portWriteErrorStatus;	// static for debug purposes only...

    status = SCBMRead(&maxScbmPort,
                       MAX_SCBM_ADDRESS,
                       buffer,
                       size,
                       &numBytesRead,
                       NULL,
                       SCBM_INF_TIMEOUT);

	portWriteErrorStatus = SCBMGetErrorStatus(&maxScbmPort);


    if ((status == SCBM_STATUS_SUCCESS) && (numBytesRead == size))
    {
        return 1;
    }
    else
    {
        scbmLastError = status;
        return 0;
    }
}





