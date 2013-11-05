/***************************************************************************************/
/* FILE: Si2153_L1_API_TV_Chassis.h                                                    */
/*                                                                                     */
/*                                                                                     */
/*                                                                                     */
/***************************************************************************************/
#ifndef Si2153_L1_API_TV_CHASSIS_H
#define Si2153_L1_API_TV_CHASSIS_H


#include "Si2153_Typedefs.h"
#include "Si2153_Commands.h"

#define NO_Si2153_ERROR                     0x00
#define ERROR_Si2153_PARAMETER_OUT_OF_RANGE 0x01
#define ERROR_Si2153_ALLOCATING_CONTEXT     0x02
#define ERROR_Si2153_SENDING_COMMAND        0x03
#define ERROR_Si2153_CTS_TIMEOUT            0x04
#define ERROR_Si2153_ERR                    0x05
#define ERROR_Si2153_POLLING_CTS            0x06
#define ERROR_Si2153_POLLING_RESPONSE       0x07
#define ERROR_Si2153_LOADING_FIRMWARE       0x08
#define ERROR_Si2153_LOADING_BOOTBLOCK      0x09
#define ERROR_Si2153_STARTING_FIRMWARE      0x0a
#define ERROR_Si2153_SW_RESET               0x0b
#define ERROR_Si2153_INCOMPATIBLE_PART		0x0c
#define ERROR_Si2153_TUNINT_TIMEOUT         0x0d
#define ERROR_Si2153_xTVINT_TIMEOUT         0x0e

unsigned char Si2153_L1_API_Init  (L1_Si2153_Context *api, L0_Context *Si2153_L0, int add);
unsigned char Si2153_L1_API_Patch (L1_Si2153_Context *api, unsigned char waitForCTS, unsigned char Si2153_waitForResponse, int iNbBytes, unsigned char *pucDataBuffer);
int			  CheckStatus  (L1_Si2153_Context *Si2153, Si2153_COMMON_REPLY_struct *status);
#endif
