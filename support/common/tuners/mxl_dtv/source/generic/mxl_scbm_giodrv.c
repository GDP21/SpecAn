/*
** FILE NAME:   $RCSfile: mxl_scbm_giodrv.c,v $
**
** TITLE:       MaxLinear tuner driver
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

#include "mxl_port.h"

#include "scbm_api.h"

/* SCBM port */
//TBD Change to be multi-context...
SCBM_PORT_T MxLScbmPort;

#ifndef SCBCLK_RATE_KHZ
#error "Must define scb clock rate."
#endif

/* SCBM settings */
static SCBM_SETTINGS_T MxLScbmSettings =
{
    400,    /* Data transfer bit rate (in kHz) */
    SCBCLK_RATE_KHZ,  /* Core clock (in kHz) */
    0,      /* Bus delay (in ns) */
    0		/* block index, default */
};

/* Last SCBM error */
static unsigned long scbmInitError = 0;

/*
** FUNCTION:    MxL_setupPort
**
** DESCRIPTION: Sets up port
**
** INPUTS:      portNumber	serial port number to use
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int MxL_setupPort(int portNumber)
{
    unsigned long status;

	switch(portNumber)
	{
			/* Valid port numbers, so set block index in settings structure to this number */
		case(0):
		case(1):
		case(2):
			MxLScbmSettings.ui32BlockIndex = portNumber;
			break;
		default:
				/* Not supported port number, so return failure */
			return 0;
			break;
	}

    status = SCBMInit(&MxLScbmPort,
                      &MxLScbmSettings,
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
** FUNCTION:    MxL_shutdownPort
**
** DESCRIPTION: Shuts down the port
**
** INPUTS:      void
**
** RETURNS:     int : 1 for success, 0 for failure
**
*/
int MxL_shutdownPort(void)
{
	/* Cancel any activity, then stop the peripheral driver */
	SCBMCancel(&MxLScbmPort);

	SCBMDeinit(&MxLScbmPort);

	return 1;
}




