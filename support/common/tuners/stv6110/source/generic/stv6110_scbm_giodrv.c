/*
** FILE NAME:   $RCSfile: stv6110_scbm_giodrv.c,v $
**
** TITLE:       Satellite tuner driver
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

#include "stv6110_port.h"

#include "scbm_api.h"

#define REGISTERADDRESS_SIZE 	1

/* SCBM port */
//TBD Change to be multi-context...
SCBM_PORT_T STV_ScbmPort;

#ifndef SCBCLK_RATE_KHZ
#error "Must define scb clock rate."
#endif

/* SCBM settings */
static SCBM_SETTINGS_T STV_ScbmSettings =
{
    400,    /* Data transfer bit rate (in kHz) */
    SCBCLK_RATE_KHZ,  /* Core clock (in kHz) */
    0,      /* Bus delay (in ns) */
    0		/* block index, default */
};

/* Any SCBM errors */
static unsigned long scbmInitError = 0, scbmLastError = 0;

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
int STV_setupPort(int portNumber)
{
    unsigned long status;

	switch(portNumber)
	{
			/* Valid port numbers, so set block index in settings structure to this number */
		case(0):
		case(1):
		case(2):
			STV_ScbmSettings.ui32BlockIndex = portNumber;
			break;
		default:
				/* Not supported port number, so return failure */
			return 0;
			break;
	}

    status = SCBMInit(&STV_ScbmPort,
                      &STV_ScbmSettings,
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
int STV_shutdownPort(void)
{
	/* Cancel any activity, then stop the peripheral driver */
	SCBMCancel(&STV_ScbmPort);

	SCBMDeinit(&STV_ScbmPort);

	return 1;
}


/*
** FUNCTION:    STV_writeMessage
**
** DESCRIPTION: Writes a message to the tuner
**
** INPUTS:      i2cAddress	I2C address of the tuner
**				buffer  	Buffer containing message to send
**              size    	Number of bytes in message
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int STV_writeMessage(int i2cAddress, unsigned char *buffer, unsigned long size)
{
    static unsigned long status, numBytesWritten, portWriteErrorStatus;	// static for debug purposes only...

	status = SCBMWrite(&STV_ScbmPort,
					   i2cAddress,
					   buffer,
					   size,
					   &numBytesWritten,
					   NULL,
					   SCBM_INF_TIMEOUT);

	portWriteErrorStatus = SCBMGetErrorStatus(&STV_ScbmPort);


	if ((status == SCBM_STATUS_SUCCESS) && (numBytesWritten == size))
	{	/* successful transfer */
		return 1;
	}

	/* Failed to send out message */
	scbmLastError = status;
	return 0;
}


/*
** FUNCTION:    STV_readMessage
**
** DESCRIPTION: Reads a message from the tuner
**
** INPUTS:      i2cAddress		I2C address of the tuner
**				registerAddress	Address of the register we are trying to read from
**				buffer  		Buffer containing message to send
**              size    		Number of bytes in message
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int STV_readMessage(int i2cAddress, unsigned char registerAddress, unsigned char *buffer, unsigned long size)
{
    unsigned long numBytesWritten;
	unsigned long status;
	unsigned long nbReadBytes;

	status = SCBMWrite(&STV_ScbmPort,
					   i2cAddress,
					   &registerAddress,
					   REGISTERADDRESS_SIZE,
					   &numBytesWritten,
					   NULL,
					   SCBM_INF_TIMEOUT);

	if ((status != SCBM_STATUS_SUCCESS) || (numBytesWritten != REGISTERADDRESS_SIZE))
	{	/* successful transfer */
		return 0;
	}

    status = SCBMRead(&STV_ScbmPort,//TBD Change to be multi-context...
                       i2cAddress,
                       buffer,
                       size,
                       &nbReadBytes,
                       NULL,
                       SCBM_INF_TIMEOUT);

	if (status != SCBM_STATUS_SUCCESS)
		nbReadBytes = 0;

    return (int)nbReadBytes;
}

/*********************************************************************************************************/
