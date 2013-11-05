/*

 Driver APIs for MxL201RF Tuner

 Copyright, Maxlinear, Inc.
 All Rights Reserved

 File Name:      MxL_User_Define.c

 */
/* Keep these first ... */
#ifdef METAG
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "MxL_User_Define.h"

#ifdef MXL5007T
#include "MxL5007_Common.h"
#else
#ifdef MXL201RF
#include "MxL201RF_Common.h"
#else
#error "Not a supported tuner."
#endif
#endif

#include "mxl_port.h"
#include "scbm_api.h"


#define	READ_MODE_ADDR_REGISTER_ADDRESS	0xFBU	/* We must write the read register address to this register address */
#define	READ_REG_ADDR_DATA_SIZE	2 /* 2 bytes write register address and address of register too read from */

/* Last SCBM error */
static unsigned long scbmLastError = 0;

/******************************************************************************
**
**  Name: MxL_I2C_Write
**
**  Description:    I2C write operations
**
**  Parameters:
**					DeviceAddr	- I2C Device address
**					pArray		- Write data array pointer
**					count		- total number of array
**
**  Returns:        MxL_OK if success
**
******************************************************************************/
UINT32 MxL_I2C_Write(UINT8 DeviceAddr, UINT8* pArray, UINT32 count)
{
//TBD Change to be multi-context...
    static unsigned long status, numBytesWritten, portWriteErrorStatus;	// static for debug purposes only...

    status = SCBMWrite(&MxLScbmPort,//TBD Change to be multi-context...
                       DeviceAddr,
                       pArray,
                       count,
                       &numBytesWritten,
                       NULL,
                       SCBM_INF_TIMEOUT);

	portWriteErrorStatus = SCBMGetErrorStatus(&MxLScbmPort); //TBD Change to be multi-context...


    if ((status == SCBM_STATUS_SUCCESS) && (numBytesWritten == count))
    {
        return MxL_OK;
    }
    else
    {
        scbmLastError = status;
        return MxL_ERR_OTHERS;
    }
}

/******************************************************************************
**
**  Name: MxL_I2C_Read
**
**  Description:    I2C read operations
**
**  Parameters:
**					DeviceAddr	- I2C Device address
**					Addr		- register address for read
**					*Data		- data return
**
**  Returns:        MxL_OK if success
**
******************************************************************************/
UINT32 MxL_I2C_Read(UINT8 DeviceAddr, UINT8 Addr, UINT8* mData)
{
	UINT8 tempWriteArray[READ_REG_ADDR_DATA_SIZE];
	UINT32 retVal;

//TBD Change to be multi-context...
    static unsigned long status, numBytesRead, portWriteErrorStatus;	// static for debug purposes only...

    tempWriteArray[0] = READ_MODE_ADDR_REGISTER_ADDRESS;
    tempWriteArray[1] = Addr;

		/* Write address to read from, into tuner. */
	retVal = MxL_I2C_Write(DeviceAddr, tempWriteArray, READ_REG_ADDR_DATA_SIZE);

		/* Pass on any error with an early return */
	if (retVal != MxL_OK)
		return(retVal);

    status = SCBMRead(&MxLScbmPort,//TBD Change to be multi-context...
                       DeviceAddr,
                       mData,
                       1,
                       &numBytesRead,
                       NULL,
                       SCBM_INF_TIMEOUT);

	portWriteErrorStatus = SCBMGetErrorStatus(&MxLScbmPort);//TBD Change to be multi-context...


    if ((status == SCBM_STATUS_SUCCESS) && (numBytesRead == 1))
    {
        return MxL_OK;
    }
    else
    {
        scbmLastError = status;
        return MxL_ERR_OTHERS;
    }
}

/******************************************************************************
**
**  Name: MxL_Delay
**
**  Description:    Delay function in milli-second
**
**  Parameters:
**					mSec		- milli-second to delay
**
******************************************************************************/
void MxL_Delay(UINT32 mSec)
{
	KRN_TASKQ_T queue;
	DQ_init(&queue);

		/* This assumes that the timer tick is 1ms */
	KRN_hibernate(&queue, mSec);
}
