/*
** FILE NAME:   dfu.c
**
** PROJECT:     Meta/DSP Software Library
**
** TITLE:       USB
**
** AUTHOR:      Ensigma Technologies, a division of Imagination Technologies
**
** DESCRIPTION: Device Firmware Upgrade Class
**
** MODIFICATION
** HISTORY:     1.0
*/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include <img_defs.h>
#include <MeOS.h>
#include <ioblock_defs.h>

/* Include files */
#include <usbd_api.h>
#include <usb_spec.h>
#include "usb_dfu.h"

#if defined USBD_NO_CBMAN_SUPPORT
	#error "DFU requires USBD CBMAN-based callback support"
#endif

/* Endian neutral read/write macros */
#define IN8(p)      (p[0])
#define IN16(p)     (p[0] + (p[1] << 8))
#define IN24(p)     (p[0] + (p[1] << 8) + (p[2] << 16))
#define IN32(p)     (p[0] + (p[1] << 8) + (p[2] << 16) + (p[3] << 24))
#define OUT8(p, x)  {*p++ = (unsigned char)x;}
#define OUT16(p, x) {*p++ = (unsigned char)(x & 0xff); *p++ = (unsigned char)((x >> 8) & 0xff);}
#define OUT24(p, x) {*p++ = (unsigned char)(x & 0xff); *p++ = (unsigned char)((x >> 8) & 0xff); *p++ = (unsigned char)((x >> 16) & 0xff);}
#define OUT32(p, x) {*p++ = (unsigned char)(x & 0xff); *p++ = (unsigned char)((x >> 8) & 0xff); *p++ = (unsigned char)((x >> 16) & 0xff); *p++ = (unsigned char)((x >> 24) & 0xff);}

/* State machine routine */
typedef IMG_INT32 (*PDFU_STATE_ROUTINE_T)( USB_CTRLREQUEST *, void *, void **);

/* Function prototypes */
IMG_INT32 DfuAppIdle( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf);

IMG_INT32 DfuAppDetatch( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf);

IMG_INT32 DfuIdle( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf);

IMG_INT32 DfuDnloadSync( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf);

IMG_INT32 DfuDnBusy( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf);

IMG_INT32 DfuDnloadIdle( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf);

IMG_INT32 DfuManifestSync( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf);

IMG_INT32 DfuManifest( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf);

IMG_INT32 DfuManifestWaitReset( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf);

IMG_INT32 DfuUploadIdle( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf);

IMG_INT32 DfuError( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf);

IMG_RESULT DfuPostControlHandler(
    img_uint32                  eEvent,
    img_void *                  pCallbackParameter,
    img_uint32                  ui32Param,
    img_void *                  pvParam
);

IMG_RESULT DfuProcessClassRequests(
    img_uint32                  eEvent,
    img_void *                  pCallbackParameter,
    img_uint32                  ui32Param,
    img_void *                  pvParam
);

IMG_INT32 DfuBuildStatus(
    USB_CTRLREQUEST *	Control,
    DFU_T *				Dfu,
    void ** 			ppvBuf,
    unsigned long 		State,
    unsigned long 		PollTimeout
    );

IMG_INT32 DfuBuildState(USB_CTRLREQUEST *Control, DFU_T *Dfu, void ** ppvBuf);

IMG_INT32 DfuBuildDownload(USB_CTRLREQUEST *Control, DFU_T *Dfu, void ** ppvBuf);

IMG_INT32 DfuBuildUpload( USB_CTRLREQUEST *Control, DFU_T *Dfu, void ** ppvBuf);

IMG_INT32 DfuStateMachine( USB_CTRLREQUEST *Control, DFU_T *Dfu, IMG_VOID ** ppvBuf, int Init);

//
// Table of state routines.
//
PDFU_STATE_ROUTINE_T DfuStateRoutines[DFU_NUM_STATES] =
{
    DfuAppIdle,
    DfuAppDetatch,
    DfuIdle,
    DfuDnloadSync,
    DfuDnBusy,
    DfuDnloadIdle,
    DfuManifestSync,
    DfuManifest,
    DfuManifestWaitReset,
    DfuUploadIdle,
    DfuError
};

/*
** FUNCTION:    DfuBuildStatus
**
** DESCRIPTION: Builds a status reply.
**
** INPUTS:      NAME        DESCRIPTION
**              Control     The control object.
**              Dfu         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuBuildStatus(
    USB_CTRLREQUEST 	*Control,
    DFU_T         		*Dfu,
    void ** 			ppvbuf,
    unsigned long 		State,
    unsigned long 		PollTimeout
    )
{
    unsigned char *p = Dfu->TempBuffer;

    Dfu->State = State;
    Dfu->PollTimeout = PollTimeout;

    OUT8(p,  Dfu->Status);
    OUT24(p, Dfu->PollTimeout);
    OUT8(p,  Dfu->State);
    OUT8(p,  Dfu->StatusString);

    //USB_CONTROL_SET_STATE(Control, USB_CONTROL_DATA);
    //USB_CONTROL_SET_TYPE(Control, USB_CONTROL_IN);        
    //USB_CONTROL_SET_COUNT(Control, 6);
    //USB_CONTROL_SET_BUFFER(Control, Dfu->TempBuffer);
    
    *ppvbuf = (IMG_VOID *) Dfu->TempBuffer;
    
	return 6;    
}

/*
** FUNCTION:    DfuBuildState
**
** DESCRIPTION: Builds a state reply.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Dfu             The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuBuildState(USB_CTRLREQUEST *Control, DFU_T *Dfu, void ** ppvBuf)
{
    unsigned char *p = Dfu->TempBuffer;

    OUT8(p, Dfu->State);

    //USB_CONTROL_SET_STATE(Control, USB_CONTROL_DATA);
    //USB_CONTROL_SET_TYPE(Control, USB_CONTROL_IN);
    //USB_CONTROL_SET_COUNT(Control, 1);
    //USB_CONTROL_SET_BUFFER(Control, Dfu->TempBuffer);
    
    *ppvBuf = Dfu->TempBuffer;
    
    return 1;
}

/*
** FUNCTION:    DfuBuildDownload
**
** DESCRIPTION: Builds a download reply.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Dfu             The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuBuildDownload(USB_CTRLREQUEST *Control, DFU_T *Dfu, void ** ppvBuf)
{
    Dfu->State = DFU_DNLOAD_SYNC;
    //USB_CONTROL_SET_STATE(Control, USB_CONTROL_DATA);
    //USB_CONTROL_SET_TYPE(Control, USB_CONTROL_OUT);
    //USB_CONTROL_SET_COUNT(Control, Dfu->Length);
    //USB_CONTROL_SET_BUFFER(Control, Dfu->Buffer);
    Dfu->DownloadTransferred = 1;
    Dfu->BlockNum = Control->wValue;
    Dfu->BytesLeft = Dfu->Length;
    
    *ppvBuf = Dfu->Buffer;
    
    return ((IMG_INT32) Dfu->Length);
}

/*
** FUNCTION:    DfuBuildUpload
**
** DESCRIPTION: Builds an upload reply.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Dfu             The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuBuildUpload( USB_CTRLREQUEST *Control, DFU_T *Dfu, void ** ppvBuf)
{
    unsigned long length;
	USBD_sBlock	*	psBlock = (USBD_sBlock *)Dfu->pvContext;

    length = Dfu->Length;
    //USB_CONTROL_SET_STATE(Control, USB_CONTROL_DATA);
    //USB_CONTROL_SET_TYPE(Control, USB_CONTROL_IN);
    //USB_CONTROL_SET_BUFFER(Control, Dfu->Buffer);
    Dfu->BlockNum = Control->wValue;
    Dfu->Event = DFU_EVENT_UPLOAD_BLOCK;

	USBD_ExecuteEventCallback(	psBlock,
								USBD_EVENT_DFU,
								0,
								(IMG_VOID *) Dfu);

    //USB_CONTROL_SET_COUNT(Control, Dfu->Length);
	Control->wLength = Dfu->Length;
    if (length == Dfu->Length)
    {
        Dfu->State = DFU_UPLOAD_IDLE;
    }
    else
    {
        Dfu->State = DFU_IDLE;
    }
    Dfu->NumBytesTransferred += Dfu->Length;
    
    *ppvBuf = Dfu->Buffer;
    
    return ((IMG_INT32) Dfu->Length);
}

/*
** FUNCTION:    DfuAppIdle
**
** DESCRIPTION: Handler for the appIDLE state.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuAppIdle( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf)
{
    DFU_T *dfu;

    IMG_ASSERT(Control != NULL);
    IMG_ASSERT(Context != NULL);

    dfu = (DFU_T *) Context;

    /* Not implemented */
    *ppvBuf = IMG_NULL;
    return 0;
}

/*
** FUNCTION:    DfuAppDetatch
**
** DESCRIPTION: Handler for the appDETATCH state.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuAppDetatch( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf)
{
    DFU_T *dfu;

    IMG_ASSERT(Control != NULL);
    IMG_ASSERT(Context != NULL);

    dfu = (DFU_T *) Context;

    /* Not implemented */
    *ppvBuf = IMG_NULL;
    return 0;
}

/*
** FUNCTION:    DfuIdle
**
** DESCRIPTION: Handler for the dfuIDLE state.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuIdle( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf)
{
    DFU_T		*	dfu;
	USBD_sBlock	*	psBlock;
    
    IMG_INT32	i32ReturnVal = 0;

    IMG_ASSERT(Control != NULL);
    IMG_ASSERT(Context != NULL);

    dfu = (DFU_T *) Context;
	psBlock = (USBD_sBlock *)dfu->pvContext;

    switch (Control->bRequest)
    {
        case DFU_DNLOAD:
            if (dfu->Length == 0 ||
                !DFU_IS_DOWNLOAD_CAPABLE(dfu->Attributes))
            {
                i32ReturnVal = DFUBuildError(dfu, ppvBuf, DFU_ERR_STALLEDPKT);
            }
            else
            {
                dfu->Event = DFU_EVENT_START_DOWNLOAD;
                
				USBD_ExecuteEventCallback(	psBlock,
											USBD_EVENT_DFU,
											0,
											(IMG_VOID *) dfu);                
                
                dfu->NumBytesTransferred = 0;
                i32ReturnVal = DfuBuildDownload(Control, dfu, ppvBuf);
            }
            break;

        case DFU_UPLOAD:
            if (!DFU_IS_UPLOAD_CAPABLE(dfu->Attributes))
            {
                i32ReturnVal = DFUBuildError(dfu, ppvBuf, DFU_ERR_STALLEDPKT);
            }
            else
            {
                dfu->Event = DFU_EVENT_START_UPLOAD;

				USBD_ExecuteEventCallback(	psBlock,
											USBD_EVENT_DFU,
											0,
											(IMG_VOID *) dfu);

                dfu->NumBytesTransferred = 0;
                i32ReturnVal = DfuBuildUpload( Control, dfu, ppvBuf);
            }
            break;

        case DFU_ABORT:
            /* Do nothing */
            break;

        case DFU_GETSTATUS:
            i32ReturnVal = DfuBuildStatus(Control, dfu, ppvBuf, dfu->State, 0);
            break;

        case DFU_GETSTATE:
            i32ReturnVal = DfuBuildState(Control, dfu, ppvBuf);
            break;

        case DFU_DISCONNECT:
            /* Disconnect from the host to take us back to app mode.
             * Note: This is not part of the DFU spec.
             */
            dfu->Disconnect = 1;
            break;

        default:
            /* Bad request */
            i32ReturnVal = DFUBuildError(dfu, ppvBuf, DFU_ERR_STALLEDPKT);
            break;
    }
    
    return i32ReturnVal;
}

/*
** FUNCTION:    DfuDnloadSync
**
** DESCRIPTION: Handler for the dfuDNLOAD-SYNC state.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuDnloadSync( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf)
{
    DFU_T *dfu;

	IMG_UINT32	i32ReturnVal = 0;

    IMG_ASSERT(Control != NULL);
    IMG_ASSERT(Context != NULL);

    dfu = (DFU_T *) Context;

    switch (Control->bRequest)
    {
        case DFU_GETSTATUS:
            if (dfu->BytesLeft != 0)
            {
                /* Block not finished */
                i32ReturnVal = DfuBuildStatus(Control, dfu, ppvBuf, DFU_DNBUSY, dfu->ProgTimeout);

                /* We don't really want to go into the busy state, so just go
                 * back to idle.
                 */
                dfu->State = DFU_DNLOAD_SYNC;
            }
            else
            {
                /* Block finished */
                i32ReturnVal = DfuBuildStatus(Control, dfu, ppvBuf, DFU_DNLOAD_IDLE, 0);
            }
            break;

        case DFU_GETSTATE:
            i32ReturnVal = DfuBuildState(Control, dfu, ppvBuf);
            break;

        case DFU_ABORT:
            dfu->State = DFU_IDLE;
            break;

        default:
            i32ReturnVal = DFUBuildError(dfu, ppvBuf, DFU_ERR_STALLEDPKT);
            break;
    }
    
    return i32ReturnVal;
}

/*
** FUNCTION:    DfuDnBusy
**
** DESCRIPTION: Handler for the dfuDNBUSY state.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuDnBusy( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf)
{
    /* Not implemented */
    *ppvBuf = IMG_NULL;
    return 0;
}

/*
** FUNCTION:    DfuDnloadIdle
**
** DESCRIPTION: Handler for the dfuDNLOAD-IDLE state.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuDnloadIdle( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf)
{
    DFU_T *dfu;

	IMG_INT32	i32ReturnVal = 0;

    IMG_ASSERT(Control != NULL);
    IMG_ASSERT(Context != NULL);

    dfu = (DFU_T *) Context;

    switch (Control->bRequest)
    {
        case DFU_DNLOAD:
            if (dfu->Length == 0)
            {
                if (dfu->NumBytesTransferred == dfu->FlashSize)
                {
                    /* This is the end of the download */
                    dfu->State = DFU_MANIFEST_SYNC;
                }
                else
                {
                    /* The host thinks its reached the end but we know
                     * otherwise.
                     */
                    i32ReturnVal = DFUBuildError(dfu, ppvBuf, DFU_ERR_NOTDONE);
                }
            }
            else
            {
                /* Another download block */
                i32ReturnVal = DfuBuildDownload(Control, dfu, ppvBuf);
            }
            break;

        case DFU_ABORT:
            dfu->State = DFU_IDLE;
            break;

        case DFU_GETSTATUS:
            i32ReturnVal = DfuBuildStatus(Control, dfu, ppvBuf, dfu->State, 0);
            break;

        case DFU_GETSTATE:
            i32ReturnVal = DfuBuildState(Control, dfu, ppvBuf);
            break;

        default:
            i32ReturnVal = DFUBuildError(dfu, ppvBuf, DFU_ERR_STALLEDPKT);
            break;
    }
    
    return i32ReturnVal;
}

/*
** FUNCTION:    DfuManifestSync
**
** DESCRIPTION: Handler for the dfuMANIFEST-SYNC state.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuManifestSync( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf)
{
    DFU_T		*	dfu;
	USBD_sBlock	*	psBlock;

	IMG_INT32	i32ReturnVal = 0;

    IMG_ASSERT(Control != NULL);
    IMG_ASSERT(Context != NULL);

    dfu = (DFU_T *) Context;
	psBlock = (USBD_sBlock *)dfu->pvContext;

    switch (Control->bRequest)
    {
        case DFU_GETSTATUS:
            dfu->Event = DFU_EVENT_MANIFEST;

			USBD_ExecuteEventCallback(	psBlock,
										USBD_EVENT_DFU,
										0,
										(IMG_VOID *) dfu);

            if (dfu->State == DFU_MANIFEST_SYNC)
            {
                i32ReturnVal = DfuBuildStatus(Control, dfu, ppvBuf, DFU_IDLE, 0);
            }
            else
            {
                i32ReturnVal = DfuBuildStatus(Control, dfu, ppvBuf, dfu->State, 0);
            }
            break;

        case DFU_GETSTATE:
            i32ReturnVal = DfuBuildState(Control, dfu, ppvBuf);
            break;

        case DFU_ABORT:
            dfu->State = DFU_IDLE;
            break;

        default:
            i32ReturnVal = DFUBuildError(dfu, ppvBuf, DFU_ERR_STALLEDPKT);
            break;
    }
    
    return i32ReturnVal;
}

/*
** FUNCTION:    DfuManifest
**
** DESCRIPTION: Handler for the dfuMANIFEST state.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuManifest( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf)
{
    /* Not implemented */
    *ppvBuf = IMG_NULL;
    return 0;
}

/*
** FUNCTION:    DfuManifestWaitReset
**
** DESCRIPTION: Handler for the dfuMANIFEST-WAIT-RESET state.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuManifestWaitReset( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf)
{
    /* Not implemented */
    *ppvBuf = IMG_NULL;
    return 0;
}

/*
** FUNCTION:    DfuUploadIdle
**
** DESCRIPTION: Handler for the dfuUPLOAD-IDLE state.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuUploadIdle( USB_CTRLREQUEST *Control, void *Context, void ** ppvBuf)
{
    DFU_T *dfu;

	IMG_INT32 i32ReturnVal = 0;

    IMG_ASSERT(Control != NULL);
    IMG_ASSERT(Context != NULL);

    dfu = (DFU_T *) Context;

    switch (Control->bRequest)
    {
        case DFU_UPLOAD:
            i32ReturnVal = DfuBuildUpload( Control, dfu, ppvBuf);
            break;

        case DFU_ABORT:
            dfu->State = DFU_IDLE;
            break;

        case DFU_GETSTATUS:
            dfu->PollTimeout = 0;
            i32ReturnVal = DfuBuildStatus( Control, dfu, ppvBuf, dfu->State, 0);
            break;

        case DFU_GETSTATE:
            i32ReturnVal = DfuBuildState(Control, dfu, ppvBuf);
            break;

        default:
            i32ReturnVal = DFUBuildError(dfu, ppvBuf, DFU_ERR_STALLEDPKT);
            break;
    }
    
    return i32ReturnVal;
}

/*
** FUNCTION:    DfuError
**
** DESCRIPTION: Handler for the dfuERROR state.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuError( USB_CTRLREQUEST *Control, void *Context, img_void ** ppvBuf)
{
    DFU_T *dfu;

	IMG_INT32	i32ReturnVal = 0;

    IMG_ASSERT(Control != NULL);
    IMG_ASSERT(Context != NULL);

    dfu = (DFU_T *) Context;

    switch (Control->bRequest)
    {
        case DFU_GETSTATUS:
            dfu->PollTimeout = 0;
            i32ReturnVal = DfuBuildStatus(Control, dfu, ppvBuf, dfu->State, 0);
            break;

        case DFU_GETSTATE:
            i32ReturnVal = DfuBuildState(Control, dfu, ppvBuf);
            break;

        case DFU_CLRSTATUS:
            dfu->Status = DFU_OK;
            dfu->State  = DFU_IDLE;
            break;

        default:
			break;
    }
    
    return i32ReturnVal;
}

/*
** FUNCTION:    DfuPostControlHandler
**
** DESCRIPTION: Post control callback.
**              This is called when a control transfer finished. We use this
**              to fire off download blocks.
**
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_RESULT DfuPostControlHandler(
    img_uint32                  eEvent,
    img_void *                  pCallbackParameter,
    img_uint32                  ui32Param,
    img_void *                  pvParam
)
{
    DFU_T		*	dfu;
	USBD_sBlock	*	psBlock;

    IMG_ASSERT(pCallbackParameter != NULL);

    dfu = (DFU_T *) pCallbackParameter;
	psBlock = (USBD_sBlock *)dfu->pvContext;

    if (dfu->DownloadTransferred)
    {
        dfu->Event = DFU_EVENT_DOWNLOAD_BLOCK;
        
		USBD_ExecuteEventCallback(	psBlock,
									USBD_EVENT_DFU,
									0,
									(IMG_VOID *) dfu);        

        dfu->DownloadTransferred = 0;
        dfu->NumBytesTransferred += dfu->Length;
    }

    if (dfu->Disconnect)
    {
        dfu->Disconnect = 0;
        dfu->State = DFU_APP_IDLE;
    }

    return 0;
}

/*
** FUNCTION:    DfuStateMachine
**
** DESCRIPTION: DFU state machine dispatcher.
**
** INPUTS:      NAME        DESCRIPTION
**              Control     Control object.
**              Dfu         DFU object.
**              Init        Are we initializing?
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DfuStateMachine( USB_CTRLREQUEST *Control, DFU_T *dfu, IMG_VOID ** ppvBuf, int Init)
{
    unsigned long oldState;
    unsigned long dfuEvent;
	USBD_sBlock	*	psBlock = (USBD_sBlock *)dfu->pvContext;
    
    IMG_INT32	i32Value = 0;

    if (!Init)
    {
        /* Record the old state */
        oldState = dfu->State;

        /* Call the state routine */
        //DBGINC(Dfu->StateRequests[Dfu->State]);
        i32Value = (DfuStateRoutines[dfu->State])( Control, dfu, ppvBuf);
    }
    else
    {
        oldState = DFU_INVALID_STATE;
    }

    /* State observer.
     * This tracks catch-all state changes.
     */
    dfuEvent = DFU_INVALID_EVENT;
    if (dfu->State == DFU_IDLE && oldState != DFU_IDLE)
    {
        dfuEvent = DFU_EVENT_IDLE;
    }
    else if (dfu->State == DFU_ERROR && oldState != DFU_ERROR)
    {
        dfuEvent = DFU_EVENT_ERROR;
    }
    if (dfuEvent != DFU_INVALID_EVENT)
    {
        dfu->Event = dfuEvent;
        
		USBD_ExecuteEventCallback(	psBlock,
									USBD_EVENT_DFU,
									0,
									(IMG_VOID *) dfu);
    }
    
    return i32Value;
}

/*
** FUNCTION:    DfuProcessClassRequests
**
** DESCRIPTION: Class request callback.
**              This is called when a class request is received.
**              We decode DFU requests in this routine.
**
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Context         The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_RESULT DfuProcessClassRequests(
    img_uint32                  eEvent,
    img_void *                  pCallbackParameter,
    img_uint32                  ui32Param,
    img_void *                  pvParam
)
{
    DFU_T			*	dfu;
    USB_CTRLREQUEST *	Control = pvParam;

    IMG_ASSERT(pCallbackParameter != NULL);    

	/* Only interested in class requests */
	if ( (((((USB_CTRLREQUEST *) pvParam)->bRequestType & 0x60) >> 5) == 1 ) )
	{
	    dfu = (DFU_T *)pCallbackParameter;
	    //DBGINC(dfu->TotalRequests);
	
	    /* Is this a DFU request? */
	    /*
	    if (Control->Setup.Normal.Type != DFU_RESERVED_BITS)
	    {
	        //
	        // This isn't a DFU request.
	        //
	        return 0;
	    }
	    */
	
	    /* Get the data length */
	    dfu->Length = Control->wLength;
	
	    /* Make the default CONTROL with no DATA */
	    //USB_CONTROL_SET_STATE(Control, USB_CONTROL_STATUS);
	
	    /* Run the DFU state machine */
	    return ((IMG_RESULT) DfuStateMachine( Control, dfu, (IMG_VOID *) ui32Param, 0));
	}
	else
	{
		*((IMG_VOID **) ui32Param) = IMG_NULL;
		return 0;
	}
}

/*
** FUNCTION:    DFUInit
**
** DESCRIPTION: Initialize a DFU object.
**
** INPUTS:      NAME            DESCRIPTION
**              Dfu             The DFU object to initialize.
**              Attributes      DFU attributes.
**              DetatchTimeout  DFU detatch timeout.
**              ProgTimeout     DFU programming timeout.
**              State           Initial state.
**              Status          Initial status.
**              Buffer          Download/upload block buffer.
**              FlashSize       The size of the Flash block.
**
** OUTPUTS:     NAME        DESCRIPTION
**              Dfu         The initialized DFU object.
**
** RETURNS:     void
*/
void DFUInit(
    DFU_T         *Dfu,
    unsigned long Attributes,
    unsigned long DetatchTimeOut,
    unsigned long ProgTimeout,
    unsigned long State,
    unsigned long Status,
    unsigned char *Buffer,
    unsigned long FlashSize
    )
{
    /* Initialize the DFU object */
    Dfu->Attributes           = Attributes;
    Dfu->DetatchTimeOut       = DetatchTimeOut;
    Dfu->ProgTimeout          = ProgTimeout;
    Dfu->Status               = Status;
    Dfu->PollTimeout          = 0;
    Dfu->State                = State;
    Dfu->StatusString         = 0;
    Dfu->DownloadTransferred  = 0;
    Dfu->Buffer               = Buffer;
    Dfu->FlashSize            = FlashSize;
    Dfu->Disconnect           = 0;
}

/*
** FUNCTION:    DFUAdd
**
** DESCRIPTION: Attached a DFU object to a control object.
**
** INPUTS:      NAME        DESCRIPTION
**              Control     The control object to attach to.
**              Dfu         The DFU object to attach.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
void DFUAdd( USBD_sBlock	*	psBlock, DFU_T *Dfu )
{	
	Dfu->pvContext = (img_void *)psBlock;

    /* Add callback handlers */
	USBD_AddEventCallback ( psBlock,
							USBD_EVENT_UNHANDLED_CONTROL_MESSAGE_RECEIVED,
							DfuProcessClassRequests,
							(IMG_VOID *) Dfu,		/* User callback parameter */
    						&(Dfu->UnhandledControlMessageCallback) );
    						
	USBD_AddEventCallback ( psBlock,
							USBD_EVENT_UNHANDLED_CONTROL_MESSAGE_COMPLETE,
							DfuPostControlHandler,
							(IMG_VOID *) Dfu,		/* User callback parameter */
    						&(Dfu->PostControlCallback) );
    
    /* Initialize the DFU state machine */
    DfuStateMachine( IMG_NULL, Dfu, IMG_NULL, 1);
}

/*
** FUNCTION:    DFURemove
**
** DESCRIPTION: Detatches a DFU object from a control object.
**
** INPUTS:      NAME        DESCRIPTION
**              Control     The control object to dettach from.
**              Dfu         The DFU object to detatch.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
void DFURemove(DFU_T *Dfu)
{
    /* Not implemented */
}

/*
** FUNCTION:    DFUBuildError
**
** DESCRIPTION: Builds an error reply.
**
** INPUTS:      NAME            DESCRIPTION
**              Control         The control object.
**              Dfu             The DFU object.
**
** OUTPUTS:     NAME        DESCRIPTION
**              None.
**
** RETURNS:     void
*/
IMG_INT32 DFUBuildError(DFU_T *Dfu, void ** ppvBuf, unsigned long Status)
{
    /* Update status */
    Dfu->Status = Status;
    *ppvBuf = IMG_NULL;

    /* If we're in application mode then move to the application state.
     * If we're in DFU mode then move to the DFU error state.
     */
    if (Dfu->State > DFU_APP_DETATCH)
    {
        Dfu->State = DFU_ERROR;
    }
    else
    {
        Dfu->State = DFU_APP_IDLE;
    }

    /* Stall the control pipe if needed */
    if (Status == DFU_ERR_STALLEDPKT)
    {
        //USB_CONTROL_SET_STATE(Control, USB_CONTROL_STALL);
        return -1;
    } 
    
    return 0;
}
