/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/msi_tuner/source/generic/msi_spim_imgworks.c,v $
**
** TITLE:       MSI tuner driver
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Implementation of SPIM port mini driver
**              This version uses IMGWorks peripheral drivers
**
** NOTICE:      Copyright (C) 2009, Imagination Technologies Ltd.
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

/* TODO: Currently SPIM with IMGWorks drivers is only used on the Unicor board.
         However, this needs making more general - maybe the SPIM port setup
         should be done as part of the board setup instead of inside the
         tuner driver.
*/
#define UNICOR_SPIM_BIT_RATE (0x32)

/* SPIM port */
static SPIM_sPort msiSpimPort;
static SPIM_eDevice msiSpimPortNum;

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
    SPIM_eReturn retVal;
    SPIM_sInitParam initParam;

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

    initParam.sDev0Param.ui8BitRate = UNICOR_SPIM_BIT_RATE;
    initParam.sDev0Param.ui8CSSetup = MSI_SPIM_CS_SETUP;
    initParam.sDev0Param.ui8CSHold = MSI_SPIM_CS_HOLD;
    initParam.sDev0Param.ui8CSDelay = MSI_SPIM_CS_DELAY;
    initParam.sDev0Param.eSPIMode = MSI_SPIM_SPI_MODE;
    initParam.sDev0Param.ui32CSIdleLevel = MSI_SPIM_CS_IDLE_LEVEL;
    initParam.sDev0Param.ui32DataIdleLevel = MSI_SPIM_DATA_IDLE_LEVEL;

    initParam.sDev1Param.ui8BitRate = UNICOR_SPIM_BIT_RATE;
    initParam.sDev1Param.ui8CSSetup = MSI_SPIM_CS_SETUP;
    initParam.sDev1Param.ui8CSHold = MSI_SPIM_CS_HOLD;
    initParam.sDev1Param.ui8CSDelay = MSI_SPIM_CS_DELAY;
    initParam.sDev1Param.eSPIMode = MSI_SPIM_SPI_MODE;
    initParam.sDev1Param.ui32CSIdleLevel = MSI_SPIM_CS_IDLE_LEVEL;
    initParam.sDev1Param.ui32DataIdleLevel = MSI_SPIM_DATA_IDLE_LEVEL;

    initParam.sDev2Param.ui8BitRate = UNICOR_SPIM_BIT_RATE;
    initParam.sDev2Param.ui8CSSetup = MSI_SPIM_CS_SETUP;
    initParam.sDev2Param.ui8CSHold = MSI_SPIM_CS_HOLD;
    initParam.sDev2Param.ui8CSDelay = MSI_SPIM_CS_DELAY;
    initParam.sDev2Param.eSPIMode = MSI_SPIM_SPI_MODE;
    initParam.sDev2Param.ui32CSIdleLevel = MSI_SPIM_CS_IDLE_LEVEL;
    initParam.sDev2Param.ui32DataIdleLevel = MSI_SPIM_DATA_IDLE_LEVEL;

    initParam.ui32DmaChannel = dmacChannel;

    retVal = SPIMInit(&msiSpimPort, &initParam);

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
    SPIM_eReturn retVal;
    SPIM_sBuffer msg;

    msg.pui8Buffer = buffer;
    msg.ui32Size = size;
    msg.i32Read = 0;   /* write */
    msg.i32Cont = 0;
    msg.eChipSelect = msiSpimPortNum;
    msg.ui8CmpValue = 0;
    msg.ui8CmpMask = 0;
    msg.iCmpEq = 0;
    msg.i32CmpData = 0;

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
