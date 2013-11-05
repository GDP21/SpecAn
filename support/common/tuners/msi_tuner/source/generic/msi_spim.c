/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/msi_tuner/source/generic/msi_spim.c,v $
**
** TITLE:       MSI tuner driver
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Implementation of SPIM port mini driver
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

#include "spim_api.h"
#include "msi_port.h"
#include "msi_spim_settings.h"

/* SPIM port */
static SPIM_PORT_T msiSpimPort;

static SPIM_DEVICE_T msiSpimPortNum;

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
    SPIM_RETURN_T retVal;
    SPIM_INIT_PARAM_T initParam;

	switch(portNumber)
	{
		case(0):
			msiSpimPortNum = SPIM_DEVICE0;
			break;
		case(1):
			msiSpimPortNum = SPIM_DEVICE1;
			break;
		case(2):
			msiSpimPortNum = SPIM_DEVICE2;
			break;
		default:
				/* Not supported port number, so return failure */
			return 0;
			break;
	}

    initParam.dev0Param.bitRate = MSI_SPIM_BIT_RATE;
    initParam.dev0Param.csSetup = MSI_SPIM_CS_SETUP;
    initParam.dev0Param.csHold = MSI_SPIM_CS_HOLD;
    initParam.dev0Param.csDelay = MSI_SPIM_CS_DELAY;
    initParam.dev0Param.spiMode = MSI_SPIM_SPI_MODE;
    initParam.dev0Param.csIdleLevel = MSI_SPIM_CS_IDLE_LEVEL;
    initParam.dev0Param.dataIdleLevel = MSI_SPIM_DATA_IDLE_LEVEL;

    initParam.dev1Param.bitRate = MSI_SPIM_BIT_RATE;
    initParam.dev1Param.csSetup = MSI_SPIM_CS_SETUP;
    initParam.dev1Param.csHold = MSI_SPIM_CS_HOLD;
    initParam.dev1Param.csDelay = MSI_SPIM_CS_DELAY;
    initParam.dev1Param.spiMode = MSI_SPIM_SPI_MODE;
    initParam.dev1Param.csIdleLevel = MSI_SPIM_CS_IDLE_LEVEL;
    initParam.dev1Param.dataIdleLevel = MSI_SPIM_DATA_IDLE_LEVEL;

    initParam.dev2Param.bitRate = MSI_SPIM_BIT_RATE;
    initParam.dev2Param.csSetup = MSI_SPIM_CS_SETUP;
    initParam.dev2Param.csHold = MSI_SPIM_CS_HOLD;
    initParam.dev2Param.csDelay = MSI_SPIM_CS_DELAY;
    initParam.dev2Param.spiMode = MSI_SPIM_SPI_MODE;
    initParam.dev2Param.csIdleLevel = MSI_SPIM_CS_IDLE_LEVEL;
    initParam.dev2Param.dataIdleLevel = MSI_SPIM_DATA_IDLE_LEVEL;

    initParam.outputDmaChannel = dmacChannel;
    initParam.inputDmaChannel  = 2; /* Not actually used as the SPIM driver has the 'read' functionality disabled */

    retVal = SPIMInit(&msiSpimPort,&initParam);

    if (retVal == SPIM_OK)
    {
        return 1;
    }
    else
    {
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
	/* No function to stop the peripheral driver */
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
    SPIM_RETURN_T retVal;
    SPIM_BUF_T msg;

    msg.buf = buffer;
    msg.size = size;
    msg.read = 0;   /* write */
    msg.cont = 0;
    msg.chipSelect = msiSpimPortNum;

    retVal = SPIMReadWrite(&msiSpimPort, &msg, NULL, NULL);

    if (retVal == SPIM_OK)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
