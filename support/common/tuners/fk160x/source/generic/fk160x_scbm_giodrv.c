/*
** FILE NAME:   $RCSfile: fk160x_scbm_giodrv.c,v $
**
** TITLE:       NuTune tuner driver
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

#include "fk160x_port.h"

#include "scbm_api.h"

#define I2C_ADDRESS	(0xc0 >> 1)
#define MAX_WRITE_DATA_SIZE	32
/* SCBM port */
//TBD Change to be multi-context...
SCBM_PORT_T FK_ScbmPort;

#ifndef SCBCLK_RATE_KHZ
#error "Must define scb clock rate."
#endif

/* SCBM settings */
static SCBM_SETTINGS_T FK_ScbmSettings =
{
    400,    /* Data transfer bit rate (in kHz) */
    SCBCLK_RATE_KHZ,  /* Core clock (in kHz) */
    0,      /* Bus delay (in ns) */
    0		/* block index, default */
};

/* Last SCBM error */
static unsigned long scbmInitError = 0;

/*
** FUNCTION:    FK_setupPort
**
** DESCRIPTION: Sets up port
**
** INPUTS:      portNumber	serial port number to use
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int FK_setupPort(int portNumber)
{
    unsigned long status;

	switch(portNumber)
	{
			/* Valid port numbers, so set block index in settings structure to this number */
		case(0):
		case(1):
		case(2):
			FK_ScbmSettings.ui32BlockIndex = portNumber;
			break;
		default:
				/* Not supported port number, so return failure */
			return 0;
			break;
	}

    status = SCBMInit(&FK_ScbmPort,
                      &FK_ScbmSettings,
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
** FUNCTION:    FK_shutdownPort
**
** DESCRIPTION: Shuts down the port
**
** INPUTS:      void
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int FK_shutdownPort(void)
{
	/* Cancel any activity, then stop the peripheral driver */
	SCBMCancel(&FK_ScbmPort);

	SCBMDeinit(&FK_ScbmPort);

	return 1;
}


//*--------------------------------------------------------------------------------------
//* Function Name       : FK_I2CRead
//* Object              :
//* Input Parameters    : 	tmUnitSelect_t tUnit
//* 						UInt32 AddrSize,
//* 						UInt8* pAddr,
//* 						UInt32 ReadLen,
//* 						UInt8* pData
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
tmErrorCode_t FK_I2CRead(tmUnitSelect_t tUnit,	UInt32 AddrSize, UInt8* pAddr,
UInt32 ReadLen, UInt8* pData)
{
    /* Variable declarations */
	static unsigned long status;
	static unsigned long nbReadBytes;
	static unsigned long nbWrittenBytes;

	/* Currently we only support a single unit with an Id of zero */
	if (tUnit != 0)
		return(I2C_ERR_BAD_UNIT_NUMBER);

	/* Set the register address */
	/* Write starting from the register addressed */
    status = SCBMWrite(&FK_ScbmPort,//TBD Change to be multi-context...
                       I2C_ADDRESS,
                       pAddr,
                       AddrSize,
                       &nbWrittenBytes,
                       NULL,
                       SCBM_INF_TIMEOUT);

	assert(status == SCBM_STATUS_SUCCESS);
    if (status != SCBM_STATUS_SUCCESS)
		return(I2C_ERR_FAILED);
    if (AddrSize != nbWrittenBytes)
		return(I2C_ERR_FAILED);

	/* Read starting from the register addressed */
    status = SCBMRead(&FK_ScbmPort,//TBD Change to be multi-context...
                       I2C_ADDRESS,
                       pData,
                       ReadLen,
                       &nbReadBytes,
                       NULL,
                       SCBM_INF_TIMEOUT);

	assert(status == SCBM_STATUS_SUCCESS);
	if (status != SCBM_STATUS_SUCCESS)
		return(I2C_ERR_FAILED);
    if (ReadLen != nbReadBytes)
		return(I2C_ERR_FAILED);

    return TM_OK;
}

//*--------------------------------------------------------------------------------------
//* Function Name       : FK_I2CWrite
//* Object              :
//* Input Parameters    : 	tmUnitSelect_t tUnit
//* 						UInt32 AddrSize,
//* 						UInt8* pAddr,
//* 						UInt32 WriteLen,
//* 						UInt8* pData
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
tmErrorCode_t FK_I2CWrite(tmUnitSelect_t tUnit, 	UInt32 AddrSize, UInt8* pAddr,
UInt32 WriteLen, UInt8* pData)
{
    /* Variable declarations */
    tmErrorCode_t err = TM_OK;

	unsigned long status;
	unsigned long nbWrittenBytes;
	unsigned long bytesToWrite = AddrSize + WriteLen;
	UInt8	writeData[MAX_WRITE_DATA_SIZE];

	/* Currently we only support a single unit with an Id of zero */
	if (tUnit != 0)
		return(I2C_ERR_BAD_UNIT_NUMBER);

	assert(bytesToWrite <= MAX_WRITE_DATA_SIZE);
	if (bytesToWrite > MAX_WRITE_DATA_SIZE)
		return(I2C_ERR_FAILED);

	memcpy(writeData,pAddr,AddrSize);
	memcpy(writeData + AddrSize,pData,WriteLen);

	/* Set the register address */

	/* Write starting from the register addressed */
    status = SCBMWrite(&FK_ScbmPort,//TBD Change to be multi-context...
                       I2C_ADDRESS,
                       writeData,
                       bytesToWrite,
                       &nbWrittenBytes,
                       NULL,
                       SCBM_INF_TIMEOUT);

	assert(status == SCBM_STATUS_SUCCESS);
    if (status != SCBM_STATUS_SUCCESS)
		err = I2C_ERR_FAILED;
    if (bytesToWrite != nbWrittenBytes)
		return(I2C_ERR_FAILED);

    return err;
}

//*--------------------------------------------------------------------------------------
//* Function Name       : FK_Wait
//* Object              :
//* Input Parameters    : 	tmUnitSelect_t tUnit
//* 						UInt32 tms
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
tmErrorCode_t FK_Wait(tmUnitSelect_t tUnit, UInt32 tms)
{
    /* Variable declarations */
    tmErrorCode_t err = TM_OK;

	KRN_TASKQ_T queue;
	DQ_init(&queue);

	(void)tUnit;

	/* This assumes that the timer tick is 1ms */
	KRN_hibernate(&queue, tms);

    return err;
}

//*--------------------------------------------------------------------------------------
//* Function Name       : FK_Print
//* Object              :
//* Input Parameters    : 	UInt32 level, const char* format, ...
//*
//* Output Parameters   : tmErrorCode_t.
//*--------------------------------------------------------------------------------------
tmErrorCode_t 			FK_Print(UInt32 level, const char* format, ...)
{
    /* Variable declarations */
    tmErrorCode_t err = TM_OK;

	(void)level;
	(void)format;

    return err;
}
