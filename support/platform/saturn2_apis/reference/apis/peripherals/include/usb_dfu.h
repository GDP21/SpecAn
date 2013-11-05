/*
** FILE NAME:	  usb_dfu.h
**
** PROJECT:	  	  Meta/DSP Software Library
**
** TITLE:		  USB Device Firmware Upgrade Class.
**
** AUTHOR:		  Ensigma Technologies, a division of Imagination Technologies
**
** DESCRIPTION:	  Public interface.
**
** MODIFICATION
** HISTORY:		  1.0
*/
#ifndef USB_DFU_H
#define USB_DFU_H

/* Include the DFU specifications */
#include "usb_dfuspec.h"

/*! This event indicates that the DFU handler has changed state */
#define     USBD_EVENT_DFU		(USBD_EVENT_CLASS_HANDLER + 0x1)

/* DFU events */
enum DFU_EVENTS
{
    DFU_EVENT_DOWNLOAD_BLOCK = 0,
    DFU_EVENT_UPLOAD_BLOCK,
    DFU_EVENT_IDLE,
    DFU_EVENT_START_DOWNLOAD,
    DFU_EVENT_START_UPLOAD,
    DFU_EVENT_MANIFEST,
    DFU_EVENT_ERROR
};

#define DFU_INVALID_EVENT 0xffffffff

/* DFU object.
 * This holds all context for a single DFU instance.
 */
typedef struct dfu_t
{
    unsigned long 		Attributes;
    unsigned long 		DetatchTimeOut;
    unsigned long 		ProgTimeout;
    unsigned long 		Status;
    unsigned long 		PollTimeout;
    unsigned long 		State;
    unsigned long 		StatusString;
    unsigned long 		Length;
    IMG_HANDLE 	  		UnhandledControlMessageCallback;
    IMG_HANDLE    		PostControlCallback;
    unsigned long 		BlockNum;
    unsigned char 		*Buffer;
    unsigned char 		TempBuffer[6];
    int           		DownloadTransferred;
    unsigned long 		NumBytesTransferred;
    unsigned long 		FlashSize;
    unsigned long 		BytesLeft;
    unsigned long 		Event;
    int           		Disconnect;
	img_void		*	pvContext;
#ifdef DBG
    unsigned long TotalRequests;
    unsigned long StateRequests[DFU_NUM_STATES];
#endif // DBG
} DFU_T;

//
// Function prototypes.
//
void DFUInit(
    DFU_T         *Dfu,
    unsigned long Attributes,
    unsigned long DetatchTimeOut,
    unsigned long ProgTimeout,
    unsigned long State,
    unsigned long Status,
    unsigned char *Buffer,
    unsigned long FlashSize
    );

void DFUAdd( USBD_sBlock	*	psBlock, DFU_T *Dfu);

void DFURemove(DFU_T *Dfu);

IMG_INT32 DFUBuildError(DFU_T *Dfu, void ** ppvBuf, unsigned long Status);

#endif /* USB_DFU_H */
