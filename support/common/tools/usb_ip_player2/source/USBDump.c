/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tools/usb_ip_player2/source/USBDump.c,v $
**
** TITLE:       USBDump
**
** AUTHOR:      Ensigma.
**
** DESCRIPTION: USB input reader
**
** NOTICE:      This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "FTD2XX.h"

#include "usb_ip.h"


static FT_HANDLE ftHandle;
static HANDLE    hEvent;
static int       initialised = 0;

static FT_HANDLE openUSBDevice(void);
static int InitUSBInput(void);


/*
** FUNCTION:	ReadUSBInput
**
** DESCRIPTION:	Reads specified number of items from USB device driver
**
** RETURNS:		Exit status.
*/

int ReadUSBInput(unsigned char *pBuffer, size_t size, int no_items, OPTIONS_S *opt)
{
    FT_STATUS ftStatus;
    DWORD EventDWord;
    DWORD RxBytes;
    DWORD TxBytes;
    DWORD BytesReceived;
	DWORD bytes_left;
	DWORD bytes_to_read;
	unsigned char *pData;


	//Initialise if not already done so
	if (!initialised)
	{
		InitUSBInput();
		initialised = 1;
	}

	//Wait for number of bytes requested                           	
	bytes_left = (DWORD)size*no_items;
	pData      = pBuffer;

	//Loop read data until requested number of bytes have been read
	while(bytes_left > 0)
	{
		//Wait for next data
        WaitForSingleObject(hEvent,INFINITE);

        // Get the number of bytes recevied etc
        FT_GetStatus(ftHandle,&RxBytes,&TxBytes,&EventDWord);

        // Read the data
        if (RxBytes > 0) 
        {
			bytes_to_read = RxBytes >= bytes_left ? bytes_left : RxBytes;
			ftStatus = FT_Read(ftHandle,pData,bytes_to_read,&BytesReceived);
            if (ftStatus == FT_OK) 
            {
				bytes_left -= bytes_to_read;
				pData += bytes_to_read;
			}
            else 
            {
                dbg_printf(SEVERE_LOG, "USB read failed!\n");
                break;      // FT_Read Failed
            }
        }
    }
	return no_items;
}

/*
** FUNCTION:	InitUSBInput
**
** DESCRIPTION:	Initialise USB input.
**
** RETURNS:		Boolean success.
*/
static int InitUSBInput(void)
{
    DWORD EventMask;
    FT_STATUS ftStatus;
 
	/* Open the device */
    ftHandle = openUSBDevice();
    if (ftHandle == NULL)
        return -1;

    /* Create event to wait on */
    hEvent = CreateEvent(NULL, FALSE, FALSE, "");   /* Create autoreset not signalled event to wait for data */
    EventMask = FT_EVENT_RXCHAR;
    ftStatus = FT_SetEventNotification(ftHandle,EventMask,hEvent);

	return 0;
}


/*
** FUNCTION:	OpenUSBDevice
**
** DESCRIPTION:	Opens a USB end-point
**
** RETURNS:		Boolean success.
*/
static FT_HANDLE openUSBDevice(void)
{
    FT_HANDLE ftHandle;
    FT_STATUS ftStatus;
    DWORD numDevs;
    char Buffer[64]; // buffer for description of first device

    /* Get the number of devices currently connected */
    ftStatus = FT_ListDevices(&numDevs,NULL,FT_LIST_NUMBER_ONLY);
    if (ftStatus != FT_OK) 
    {
        dbg_printf(SEVERE_LOG, "Failed to open USB device!\n");    // FT_ListDevices failed
        return NULL;
    }

    /* List the device description */
    dbg_printf(INFO_LOG, "Found %d USB devices\n", numDevs);
    if (numDevs == 0)
    {
        dbg_printf(SEVERE_LOG, "Failed to open USB device!\n");    
        return NULL;
    }
    numDevs = 1;
    ftStatus = FT_ListDevices(&numDevs, &Buffer, FT_LIST_BY_INDEX|FT_OPEN_BY_DESCRIPTION);
    if (ftStatus == FT_OK) {
        dbg_printf(INFO_LOG, "Opening '%s' USB device.\n", Buffer);    
    }

    /* Obtain a handle to the device */
    ftStatus = FT_Open(0,&ftHandle);
    if (ftStatus == FT_OK) {
        return ftHandle;    // FT_Open OK, use ftHandle to access device
    }
    else {
        dbg_printf(SEVERE_LOG, "Failed to open USB device!\n");    
        return NULL;
    }
}
