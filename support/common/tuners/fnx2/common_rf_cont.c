/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/fnx2/common_rf_cont.c,v $
**
** TITLE:       Common RF Control
**
** AUTHOR:      Futurewaves, Peter Cheung
**
** DESCRIPTION: Common control functions used by RF variants for XENIF
**
**
*/

/*
** Include these before any other include to ensure TBI used correctly
** All METAG and METAC values from machine.inc and then tbi.
*/
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>

#include "common_rf_cont.h"


#define WRITE(a,d)    (*(volatile unsigned long *)(a) = (d))
#define READ(a)       (*(volatile unsigned long *)(a))



////////////////////////////////////////////////////////////////////
////////////include .h files and define structure///////////////////
////////////////////////////////////////////////////////////////////

///xenif SoC -> I2C tuner////
#include "system.h"
#include "sys_config.h"
#include "scbm_api.h"

#define SCBM_BITRATE_400K  (400) //fast I2C mode of 400kbps [50..400] in unit of kHz //peter 10Jun2008
#define SCBM_BITRATE_300K  (350)
#define SCBM_BITRATE_100K  (100)
#define DELAYMS				(1)
static SCBM_PORT_T     scbMasterPort;
#define NUM_IO_BLOCKS		(8)
static SCBM_IO_BLOCK_T scbIOblock[NUM_IO_BLOCKS];
#define SCBM_TEST_STATUS_ASYNC_DEFAULT (0xFF)

typedef struct scbm_test_async_ctx_tag
{
	IMG_VOID	*	pContext;
	IMG_INT32		i32Read;
	IMG_UINT32		ui32Address;
	IMG_UINT8	*	pui8Buffer;
	IMG_UINT32		ui32NumBytesTransferred;
	IMG_UINT32		ui32Status;
} TEST_sAsyncCtx;


//#define SCBM_DEBUG  //peter 04Oct2007
#define RF_CONTROL_SCB  SCBM_PORT_1
static SCBM_SETTINGS_T scbMasterSettings;
unsigned long I2Cresult;  //peter 04Oct2007
unsigned char Flag_SCBM_init=0;


/******************************************************************************
	Internal function prototypes
 ******************************************************************************/
static void	SCB_callback(	void			*	pContext,
							long				i32Read,
							unsigned long		ui32Address,
							unsigned char	*	pui8Buffer,
							unsigned long		ui32NumBytesTransferred,
							unsigned long		ui32Status );
														
//===============================================================================================================



////////////////////////////////////////////////////////
/*!
******************************************************************************

 @Function				SCB_callback

******************************************************************************/


static void	SCB_callback(	void			*	pContext,
							long				i32Read,
							unsigned long		ui32Address,
							unsigned char	*	pui8Buffer,
							unsigned long		ui32NumBytesTransferred,
							unsigned long		ui32Status )
{
		TEST_sAsyncCtx *psAsyncCtx = (TEST_sAsyncCtx *)pContext;
  	
		psAsyncCtx->pContext				= pContext;
		psAsyncCtx->i32Read					= i32Read;
		psAsyncCtx->ui32Address				= ui32Address;
		psAsyncCtx->pui8Buffer				= pui8Buffer;
		psAsyncCtx->ui32NumBytesTransferred	= ui32NumBytesTransferred;
		psAsyncCtx->ui32Status				= ui32Status;
	return;
}



/*!
******************************************************************************
** FUNCTION:    RFContolInit
**
** DESCRIPTION: This function initialises the resources used to control
**              access to the RF via SCB or SPI.
**
** INPUTS:      void
**
** RETURNS:     void
**
*/

void RFContolInit(unsigned int coreclock)
{						
    scbMasterSettings.bitrate = SCBM_BITRATE_400K;            
	scbMasterSettings.coreclock=coreclock;    		
	scbMasterSettings.busdelay=10;				
    //there is only 1 port available with 1 IO block    
    if (Flag_SCBM_init == 0)    
    {		
    		Flag_SCBM_init = 1;
    		I2Cresult= SCBMInit(&scbMasterPort, RF_CONTROL_SCB, &scbMasterSettings, scbIOblock, NUM_IO_BLOCKS);
    }
    if (SCBM_STATUS_SUCCESS != I2Cresult)
	{
		__TBILogF("asynch IO transfer FAIL: init I2C master FAIL with code=%d\n",I2Cresult);
		exit(-1);
	}
	//__TBILogF("asynch IO transfer OK: I2C Master init OK with bit rate %dkbps \n",scbMasterSettings.bitrate);
			

}




/*
** FUNCTION:    RFContolMessage
**
** DESCRIPTION: This function sends/gets bytes to/from SCB/SPI driver.
**
** INPUTS:      *buf         - pointer to array of bytes to be sent
**					FOR I2C 
**						buf[0]=I2C slave address (7bit subaddress + W/R)
**						buf[1]=I2C register address
**						buf[2]=I2C W/R data
**               size        - number of bytes in array 
**					(3 byte for I2C)
**               read_nwrite - read/write flag
**
** RETURNS:     void
**
*/
void RFContolMessage(unsigned char *buf, short size, int read_nwrite)
{							
	KRN_TASKQ_T g_SCBQueue;	
	TEST_sAsyncCtx asyncCallbackCtxA;
	IMG_UINT8	g_ReadBuffer[3];
    SCBM_ASYNC_T async;
    unsigned long num_transferred;
    unsigned char address;


    //use macro to configure asynchronous callback operation   
    DQ_init(&g_SCBQueue);        

    //at this point, buf[0] contains the slave address
    //size is the total length of transfer, including address byte
    //thus we extract the address from buf and decrement the size
    address = (buf[0] & 0xFE);
    g_ReadBuffer[0]=buf[1];
    g_ReadBuffer[1]=buf[2];        

		size -= 1;
    
    if (read_nwrite) ///////read operation/////////
    {
		asyncCallbackCtxA.ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;
    	SCBM_ASYNC_CALLBACK(&async,(SCBM_CALLBACKROUTINE_T*)&SCB_callback, &asyncCallbackCtxA);  
	   if (SCBM_STATUS_SUCCESS != SCBMWrite(&scbMasterPort, address, g_ReadBuffer, 1 , &num_transferred, &async, SCBM_INF_TIMEOUT))	//dummy write	   	
	   {
	   			__TBILogF("FAIL:  I2C dummy W during R operation\n");
					exit(-1);										
	   }
	  	while (asyncCallbackCtxA.ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
			{

				KRN_hibernate(&g_SCBQueue, DELAYMS);  //change to 1ms
			}	   	   	   	   
			//KRN_hibernate(&g_SCBQueue, DELAYMS);  


		if (asyncCallbackCtxA.ui32Status != SCBM_STATUS_SUCCESS)
		{
				__TBILogF("FAIL: I2C dummy W during R operation callback had incorrect status! (status = %d)\n", asyncCallbackCtxA.ui32Status);
				exit(-1);
		}
		
		asyncCallbackCtxA.ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;	            
		SCBM_ASYNC_CALLBACK(&async,(SCBM_CALLBACKROUTINE_T*)&SCB_callback, &asyncCallbackCtxA); 			
		if (SCBM_STATUS_SUCCESS != SCBMRead(&scbMasterPort, address, g_ReadBuffer, 1 , &num_transferred, &async, SCBM_INF_TIMEOUT))
		{	
				__TBILogF("FAIL:  I2C Read operation\n");									
				exit(-1);	
		}
			while (asyncCallbackCtxA.ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
			{

				KRN_hibernate(&g_SCBQueue, DELAYMS);  //change to 1ms
			}	   	   	 	
	 		//KRN_hibernate(&g_SCBQueue, DELAYMS);

		buf[2]=g_ReadBuffer[0];

	}
	else  ///////write operation//////////////
	{
		
		// need to modify here for sync operation
		//if (SCBMWrite(&g_sPort, g_sTestDescription.sPortA.ui32SlaveAddress, g_testAddressBuffer, 1, &g_ui32PortANumWrite,
		//				NULL, SCBM_INF_TIMEOUT) != SCBM_STATUS_SUCCESS)

// Testing sync		
//			if (SCBM_STATUS_SUCCESS != SCBMWrite(&scbMasterPort, address, g_ReadBuffer, size, &num_transferred, NULL, SCBM_INF_TIMEOUT))    	
//    	{	
//    			__TBILogF("FAIL:  I2C Write operation\n");
//					exit(-1);					
//			}				
		
																		

			asyncCallbackCtxA.ui32Status = SCBM_TEST_STATUS_ASYNC_DEFAULT;
    	SCBM_ASYNC_CALLBACK(&async,(SCBM_CALLBACKROUTINE_T*)&SCB_callback, &asyncCallbackCtxA);  
    	if (SCBM_STATUS_SUCCESS != SCBMWrite(&scbMasterPort, address, g_ReadBuffer, size, &num_transferred, &async, SCBM_INF_TIMEOUT))    	
    	{	
    		__TBILogF("FAIL:  I2C Write operation\n");
			exit(-1);					
		}				
		while (asyncCallbackCtxA.ui32Status == SCBM_TEST_STATUS_ASYNC_DEFAULT)
		{
				KRN_hibernate(&g_SCBQueue, DELAYMS);  //change to 1ms
		}	   	   	 	
	 		//KRN_hibernate(&g_SCBQueue, DELAYMS);																						 		
	}
    
}


