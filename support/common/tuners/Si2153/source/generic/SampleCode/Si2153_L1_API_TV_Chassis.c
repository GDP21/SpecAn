/***************************************************************************************/
/* FILE: Si2153_L1_API_TV_Chassis.c                                                    */
/*                                                                                     */
/*                                                                                     */
/*                                                                                     */
/***************************************************************************************/
#include <stdlib.h>
#include "Si2153_Typedefs.h"
#include "Si2153_L0_TV_Chassis.h"
#include "Si2153_L1_API_TV_Chassis.h"
/***********************************************************************************************************************
  NAME: Si2153_L1_API_Init
  DESCRIPTION: software initialisation function
              Used to initialize the software context
  Returns:    0 if no error
  Comments:   It should be called first and once only when starting the application
  Parameter:  *api    a pointer to the api context to initialize
  Parameter:  *Si2153_L0 a pointer to the L0 Context
  Parameter:  add  the Si2153 I2C address
 ***********************************************************************************************************************/
unsigned char Si2153_L1_API_Init  (L1_Si2153_Context *api, L0_Context *Si2153_L0, int add)
{

   L0_Init(Si2153_L0);

   api->i2c   = Si2153_L0;
    api->i2c->address = add;

    return NO_Si2153_ERROR;
}
/***********************************************************************************************************************
  NAME: Si2153_L1_API_Patch
  DESCRIPTION: Patch information function
              Used to send a number of bytes to the Si2153. Useful to download the firmware.
  Parameter:   *api    a pointer to the api context to initialize
  Parameter:  waitForCTS flag for CTS checking prior to sending a Si2153 API Command
  Parameter:  waitForResponse flag for CTS checking and Response readback after sending Si2153 API Command
  Parameter:  number of bytes to transmit
  Parameter:  Databuffer containing the bytes to transfer in an unsigned char array.
  Returns:   0 if no error, else a nonzero int representing an error
 ***********************************************************************************************************************/
unsigned char Si2153_L1_API_Patch (L1_Si2153_Context *api, unsigned char waitForCTS, unsigned char Si2153_waitForResponse, int iNbBytes, unsigned char *pucDataBuffer) {
    unsigned char res=0;
    unsigned char error_code=0;
    unsigned char rspByteBuffer[1];
    Si2153_COMMON_REPLY_struct status;

    if (waitForCTS)
    {
        res = Si2153_pollForCTS(api, waitForCTS);
        if (res != NO_Si2153_ERROR)
        {
            return res;
        }
    }

    res = L0_WriteCommandBytes(api->i2c, iNbBytes, pucDataBuffer);
    if (res!=iNbBytes)
       return ERROR_Si2153_SENDING_COMMAND;


    if (Si2153_waitForResponse)
    {
        error_code = Si2153_pollForResponse(api, Si2153_waitForResponse, 1, rspByteBuffer, &status);
    }
    return error_code;
}
/************************************************************************************************************************
  NAME: CheckStatus
  DESCRIPTION:     Read Si2153 STATUS byte and return decoded status
  Parameter:  Si2153 Context (I2C address)
  Parameter:  Status byte (TUNINT, ATVINT, DTVINT, ERR, CTS, CHLINT, and CHL flags).
  Returns:    Si2153/I2C transaction error code
************************************************************************************************************************/
int CheckStatus  (L1_Si2153_Context *Si2153, Si2153_COMMON_REPLY_struct *status)
{
    unsigned char buffer[1];
	/* read STATUS byte */
    if (Si2153_pollForResponse(Si2153, 1, 1, buffer, status) != 0)
	{
        return ERROR_Si2153_POLLING_RESPONSE;
    }

    return 0;
}
/**********************************************************************************************************************/
