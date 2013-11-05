/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/msi_tuner/source/generic/msi_scbm.c,v $
**
** TITLE:       MSI tuner driver
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Implementation of SCBM port mini driver
**
** NOTICE:      Copyright (C) 2007-2009, Imagination Technologies Ltd.
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

#include "scbm_api.h"
#include "msi_port.h"

/* I2C address of the MSI tuner */
#define MSI_SCBM_ADDRESS (0x4c << 1)

/* SCBM port */
static SCBM_PORT_T msiScbmPort;

/* SCBM settings */
static SCBM_SETTINGS_T msiScbmSettings =
{
    400,    /* Data transfer bit rate (in kHz) */
    169000, /* Core clock (in kHz) */
    0       /* Bus delay (in ns) */
};

/* Last SCBM error */
static unsigned long scbmLastError = 0;

static enum SCBM_PORT_NUMBERS msiScbmPortNum;

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
		case(1):
			msiScbmPortNum = SCBM_PORT_1;
			break;
#if 0
		/* Only port 1 supported. */
		case(2):
			msiScbmPortNum = SCBM_PORT_2;
			break;
#endif
		default:
				/* Not supported port number, so return failure */
			return 0;
			break;
	}

    status = SCBMInit(&msiScbmPort,
                      msiScbmPortNum,
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
	/* Cancel any activity, but no function to stop the peripheral driver */
	SCBMCancel(&msiScbmPort);

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
    unsigned long status, numBytesWritten;

    status = SCBMWrite(&msiScbmPort,
                       MSI_SCBM_ADDRESS,
                       buffer,
                       size,
                       &numBytesWritten,
                       NULL,
                       SCBM_INF_TIMEOUT);

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
