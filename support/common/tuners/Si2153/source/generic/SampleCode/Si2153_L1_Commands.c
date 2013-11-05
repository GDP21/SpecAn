/***************************************************************************************/
/* FILE: Si2153_L1_Commands.c                                                          */
/*                                                                                     */
/*                                                                                     */
/*                                                                                     */
/***************************************************************************************/
/*************************************************************************************************************/
/*                     Silicon Laboratories Broadcast API Layer 1                                        */
/*************************************************************************************************************/
#include "Si2153_Typedefs.h"
#include "Si2153_L1_API_TV_Chassis.h"
#include "Si2153_Commands.h"
#include "Si2153_L0_TV_Chassis.h"
#define Si2153_COMMAND_PROTOTYPES
#define DEBUG_RANGE_CHECK
Si2153_COMMON_REPLY_struct status;

/***********************************************************************************************************************
  Si2153_CurrentResponseStatus function
  Use:        status checking function
              Used to fill the Si2153_COMMON_REPLY_struct members with the ptDataBuffer byte's bits
  Comments:   The status byte definition being identical for all commands,
              using this function to fill the status structure hels reducing the code size

  Parameter: *ret          the Si2153_COMMON_REPLY_struct
  Parameter: ptDataBuffer  a single byte received when reading a command's response (the first byte)
  Returns:   0 if the err bit (bit 6) is unset, 1 otherwise
 ***********************************************************************************************************************/
unsigned char Si2153_CurrentResponseStatus (Si2153_COMMON_REPLY_struct *ret, unsigned char ptDataBuffer) {

    ret->tunint = ((ptDataBuffer >> 0 ) & 0x01);
    ret->atvint = ((ptDataBuffer >> 1 ) & 0x01);
    ret->dtvint = ((ptDataBuffer >> 2 ) & 0x01);
    ret->err    = ((ptDataBuffer >> 6 ) & 0x01);
    ret->cts    = ((ptDataBuffer >> 7 ) & 0x01);

    return (ret->err ? ERROR_Si2153_ERR : NO_Si2153_ERROR);
}

/***********************************************************************************************************************
  Si2153_pollForCTS function
  Use:        CTS checking function
              Used to check the CTS bit until it is set before sending the next command
  Comments:   The status byte definition being identical for all commands,
              using this function to fill the status structure hels reducing the code size
  Comments:   waitForCTS = 1 => I2C polling
              waitForCTS = 2 => INTB followed by a read (reading a HW byte using the cypress chip)
              max timeout = 100 ms

  Porting:    If reading INTB is not possible, the waitForCTS = 2 case can be removed

  Parameter: waitForCTS          a flag indicating if waiting for CTS is required
  Returns:   1 if the CTS bit is set, 0 otherwise
 ***********************************************************************************************************************/
unsigned char Si2153_pollForCTS (L1_Si2153_Context *context, unsigned char waitForCTS) {
    unsigned char error_code;
    unsigned char loop_count;
    unsigned char rspByteBuffer[1];

	for (loop_count=0;loop_count<50;loop_count++)	{		/* wait a maximum of 50*25ms = 1.25s                        */
		switch (waitForCTS)	{				/* type of CTS polling?                                     */
		case 0 :							/* no polling? valid option, but shouldn't have been called */
			error_code = NO_Si2153_ERROR;			/* return no error                                          */
			goto exit;

		case 1 :							/* I2C polling status?                                      */
			if (L0_ReadCommandBytes(context->i2c, 1, rspByteBuffer) != 1)
        error_code = ERROR_Si2153_POLLING_CTS;
      else
        error_code = NO_Si2153_ERROR;
   			if (error_code || (rspByteBuffer[0] & 0x80))
				goto exit;
			break;

		default :
			error_code = ERROR_Si2153_PARAMETER_OUT_OF_RANGE; /* support debug of invalid CTS poll parameter   */
			goto exit;

		}
		system_wait(2);                    /* CTS not set, wait 2ms and retry                         */
	}
	error_code = ERROR_Si2153_CTS_TIMEOUT;

exit:
   return error_code;
}

/***********************************************************************************************************************
  Si2153_pollForResponse function
  Use:        command response retrieval function
              Used to retrieve the command response in the provided buffer,
              poll for response either by I2C polling or wait for INTB
  Comments:   The status byte definition being identical for all commands,
              using this function to fill the status structure hels reducing the code size
  Comments:   waitForCTS = 1 => I2C polling
              waitForCTS = 2 => INTB followed by a read (reading a HW byte using the cypress chip)
              max timeout = 100 ms

  Porting:    If reading INTB is not possible, the waitForCTS = 2 case can be removed

  Parameter:  waitForResponse  a flag indicating if waiting for the response is required
  Parameter:  nbBytes          the number of response bytes to read
  Parameter:  pByteBuffer      a buffer into which bytes will be stored
  Returns:    0 if no error, an error code otherwise
 ***********************************************************************************************************************/
unsigned char Si2153_pollForResponse (L1_Si2153_Context *context, unsigned char waitForResponse, unsigned int nbBytes, unsigned char *pByteBuffer, Si2153_COMMON_REPLY_struct *status) {
    unsigned char error_code;
    unsigned char loop_count;

	for (loop_count=0;loop_count<50;loop_count++)	{		/* wait a maximum of 50*2ms = 100 ms                        */
		switch (waitForResponse)	{		/* type of response polling?                                */
		case 0 :							/* no polling? valid option, but shouldn't have been called */
			error_code = NO_Si2153_ERROR;			/* return no error                                          */
			goto exit;

		case 1 :							/* I2C polling status?                                      */
			if (L0_ReadCommandBytes(context->i2c, nbBytes, pByteBuffer) != (int)nbBytes) error_code = ERROR_Si2153_POLLING_RESPONSE;
            else error_code = NO_Si2153_ERROR;
			if (error_code)	goto exit;	/* if error, exit with error code                           */
			if (pByteBuffer[0] & 0x80)	{ /* CTS set?                                                 */
                error_code = Si2153_CurrentResponseStatus(status, pByteBuffer[0]);
				goto exit;					/* exit whether ERR set or not                              */
			}
			break;

	#ifdef   DEBUG_RANGE_CHECK
			default :
				error_code = ERROR_Si2153_PARAMETER_OUT_OF_RANGE; /* support debug of invalid CTS poll parameter   */
				goto exit;
	#endif /* DEBUG_RANGE_CHECK */
		}
		system_wait(2);                    /* CTS not set, wait 2ms and retry                         */
	}
	error_code = ERROR_Si2153_CTS_TIMEOUT;

exit:
   return error_code;
}

/* _commands_insertion_start */
#ifdef    Si2153_AGC_OVERRIDE_CMD
 /*---------------------------------------------------*/
/* Si2153_AGC_OVERRIDE COMMAND                     */
/*---------------------------------------------------*/
unsigned char Si2153_L1_AGC_OVERRIDE              (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   force_max_gain,
                                                                                      unsigned char   force_top_gain,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[2];
    unsigned char rspByteBuffer[1];

  #ifdef   DEBUG_RANGE_CHECK
	if ( (force_max_gain > Si2153_AGC_OVERRIDE_CMD_FORCE_MAX_GAIN_MAX)
	  || (force_top_gain > Si2153_AGC_OVERRIDE_CMD_FORCE_TOP_GAIN_MAX) )
	return ERROR_Si2153_PARAMETER_OUT_OF_RANGE;
  #endif /* DEBUG_RANGE_CHECK */

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_AGC_OVERRIDE_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( force_max_gain & Si2153_AGC_OVERRIDE_CMD_FORCE_MAX_GAIN_MASK ) << Si2153_AGC_OVERRIDE_CMD_FORCE_MAX_GAIN_LSB|
                          ( force_top_gain & Si2153_AGC_OVERRIDE_CMD_FORCE_TOP_GAIN_MASK ) << Si2153_AGC_OVERRIDE_CMD_FORCE_TOP_GAIN_LSB);

    if (L0_WriteCommandBytes(context->i2c, 2, cmdByteBuffer) != 2) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 1, rspByteBuffer, &status);
        rsp->agc_override.STATUS = &status;
    }
  exit:
    return error_code;
}
#endif /* Si2153_AGC_OVERRIDE_CMD */
#ifdef    Si2153_ATV_CW_TEST_CMD
 /*---------------------------------------------------*/
/* Si2153_ATV_CW_TEST COMMAND                      */
/*---------------------------------------------------*/
unsigned char Si2153_L1_ATV_CW_TEST   (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   pc_lock,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[2];
    unsigned char rspByteBuffer[1];

  #ifdef   DEBUG_RANGE_CHECK
	if ( (pc_lock > Si2153_ATV_CW_TEST_CMD_PC_LOCK_MAX) )
	return ERROR_Si2153_PARAMETER_OUT_OF_RANGE;
  #endif /* DEBUG_RANGE_CHECK */

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_ATV_CW_TEST_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( pc_lock & Si2153_ATV_CW_TEST_CMD_PC_LOCK_MASK ) << Si2153_ATV_CW_TEST_CMD_PC_LOCK_LSB);

    if (L0_WriteCommandBytes(context->i2c, 2, cmdByteBuffer) != 2) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 1, rspByteBuffer, &status);
        rsp->atv_cw_test.STATUS = &status;
    }
  exit:
    return error_code;
}
#endif /* Si2153_ATV_CW_TEST_CMD */
#ifdef    Si2153_ATV_RESTART_CMD
 /*---------------------------------------------------*/
/* Si2153_ATV_RESTART COMMAND                      */
/*---------------------------------------------------*/
unsigned char Si2153_L1_ATV_RESTART               (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,

                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[1];
    unsigned char rspByteBuffer[1];

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_ATV_RESTART_CMD;

    if (L0_WriteCommandBytes(context->i2c, 1, cmdByteBuffer) != 1) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 1, rspByteBuffer, &status);
        rsp->atv_restart.STATUS = &status;
    }
  exit:
    return error_code;
}
#endif /* Si2153_ATV_RESTART_CMD */
#ifdef    Si2153_ATV_STATUS_CMD
 /*---------------------------------------------------*/
/* Si2153_ATV_STATUS COMMAND                       */
/*---------------------------------------------------*/
unsigned char Si2153_L1_ATV_STATUS                (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   intack,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[2];
    unsigned char rspByteBuffer[9];

  #ifdef   DEBUG_RANGE_CHECK
	if ( (intack > Si2153_ATV_STATUS_CMD_INTACK_MAX) )
	return ERROR_Si2153_PARAMETER_OUT_OF_RANGE;
  #endif /* DEBUG_RANGE_CHECK */

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_ATV_STATUS_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( intack & Si2153_ATV_STATUS_CMD_INTACK_MASK ) << Si2153_ATV_STATUS_CMD_INTACK_LSB);

    if (L0_WriteCommandBytes(context->i2c, 2, cmdByteBuffer) != 2) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 9, rspByteBuffer, &status);
        rsp->atv_status.STATUS = &status;
        if (!error_code)  {
          rsp->atv_status.chlint    =   (( ( (rspByteBuffer[1]  )) >> Si2153_ATV_STATUS_RESPONSE_CHLINT_LSB    ) & Si2153_ATV_STATUS_RESPONSE_CHLINT_MASK    );
          rsp->atv_status.pclint    =   (( ( (rspByteBuffer[1]  )) >> Si2153_ATV_STATUS_RESPONSE_PCLINT_LSB    ) & Si2153_ATV_STATUS_RESPONSE_PCLINT_MASK    );
          rsp->atv_status.chl       =   (( ( (rspByteBuffer[2]  )) >> Si2153_ATV_STATUS_RESPONSE_CHL_LSB       ) & Si2153_ATV_STATUS_RESPONSE_CHL_MASK       );
          rsp->atv_status.pcl       =   (( ( (rspByteBuffer[2]  )) >> Si2153_ATV_STATUS_RESPONSE_PCL_LSB       ) & Si2153_ATV_STATUS_RESPONSE_PCL_MASK       );
          rsp->atv_status.afc_freq  = (((( ( (rspByteBuffer[4]  ) | (rspByteBuffer[5]  << 8 )) >> Si2153_ATV_STATUS_RESPONSE_AFC_FREQ_LSB  ) & Si2153_ATV_STATUS_RESPONSE_AFC_FREQ_MASK) <<Si2153_ATV_STATUS_RESPONSE_AFC_FREQ_SHIFT ) >>Si2153_ATV_STATUS_RESPONSE_AFC_FREQ_SHIFT  );
          rsp->atv_status.video_sys =   (( ( (rspByteBuffer[8]  )) >> Si2153_ATV_STATUS_RESPONSE_VIDEO_SYS_LSB ) & Si2153_ATV_STATUS_RESPONSE_VIDEO_SYS_MASK );
          rsp->atv_status.color     =   (( ( (rspByteBuffer[8]  )) >> Si2153_ATV_STATUS_RESPONSE_COLOR_LSB     ) & Si2153_ATV_STATUS_RESPONSE_COLOR_MASK     );
          rsp->atv_status.trans     =   (( ( (rspByteBuffer[8]  )) >> Si2153_ATV_STATUS_RESPONSE_TRANS_LSB     ) & Si2153_ATV_STATUS_RESPONSE_TRANS_MASK     );
        }
    }
  exit:
    return error_code;
}
#endif /* Si2153_ATV_STATUS_CMD */
#ifdef    Si2153_CONFIG_PINS_CMD
 /*---------------------------------------------------*/
/* Si2153_CONFIG_PINS COMMAND                      */
/*---------------------------------------------------*/
unsigned char Si2153_L1_CONFIG_PINS               (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   gpio1_mode,
                                                                                      unsigned char   gpio1_read,
                                                                                      unsigned char   gpio2_mode,
                                                                                      unsigned char   gpio2_read,
                                                                                      unsigned char   gpio3_mode,
                                                                                      unsigned char   gpio3_read,
                                                                                      unsigned char   bclk1_mode,
                                                                                      unsigned char   bclk1_read,
                                                                                      unsigned char   xout_mode,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[6];
    unsigned char rspByteBuffer[6];

  #ifdef   DEBUG_RANGE_CHECK
	if ( (gpio1_mode > Si2153_CONFIG_PINS_CMD_GPIO1_MODE_MAX)
	  || (gpio1_read > Si2153_CONFIG_PINS_CMD_GPIO1_READ_MAX)
	  || (gpio2_mode > Si2153_CONFIG_PINS_CMD_GPIO2_MODE_MAX)
	  || (gpio2_read > Si2153_CONFIG_PINS_CMD_GPIO2_READ_MAX)
	  || (gpio3_mode > Si2153_CONFIG_PINS_CMD_GPIO3_MODE_MAX)
	  || (gpio3_read > Si2153_CONFIG_PINS_CMD_GPIO3_READ_MAX)
	  || (bclk1_mode > Si2153_CONFIG_PINS_CMD_BCLK1_MODE_MAX)
	  || (bclk1_read > Si2153_CONFIG_PINS_CMD_BCLK1_READ_MAX)
	  || (xout_mode  > Si2153_CONFIG_PINS_CMD_XOUT_MODE_MAX ) )
	return ERROR_Si2153_PARAMETER_OUT_OF_RANGE;
  #endif /* DEBUG_RANGE_CHECK */

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_CONFIG_PINS_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( gpio1_mode & Si2153_CONFIG_PINS_CMD_GPIO1_MODE_MASK ) << Si2153_CONFIG_PINS_CMD_GPIO1_MODE_LSB|
                          ( gpio1_read & Si2153_CONFIG_PINS_CMD_GPIO1_READ_MASK ) << Si2153_CONFIG_PINS_CMD_GPIO1_READ_LSB);
    cmdByteBuffer[2] = (unsigned char) ( ( gpio2_mode & Si2153_CONFIG_PINS_CMD_GPIO2_MODE_MASK ) << Si2153_CONFIG_PINS_CMD_GPIO2_MODE_LSB|
                          ( gpio2_read & Si2153_CONFIG_PINS_CMD_GPIO2_READ_MASK ) << Si2153_CONFIG_PINS_CMD_GPIO2_READ_LSB);
    cmdByteBuffer[3] = (unsigned char) ( ( gpio3_mode & Si2153_CONFIG_PINS_CMD_GPIO3_MODE_MASK ) << Si2153_CONFIG_PINS_CMD_GPIO3_MODE_LSB|
                          ( gpio3_read & Si2153_CONFIG_PINS_CMD_GPIO3_READ_MASK ) << Si2153_CONFIG_PINS_CMD_GPIO3_READ_LSB);
    cmdByteBuffer[4] = (unsigned char) ( ( bclk1_mode & Si2153_CONFIG_PINS_CMD_BCLK1_MODE_MASK ) << Si2153_CONFIG_PINS_CMD_BCLK1_MODE_LSB|
                          ( bclk1_read & Si2153_CONFIG_PINS_CMD_BCLK1_READ_MASK ) << Si2153_CONFIG_PINS_CMD_BCLK1_READ_LSB);
    cmdByteBuffer[5] = (unsigned char) ( ( xout_mode  & Si2153_CONFIG_PINS_CMD_XOUT_MODE_MASK  ) << Si2153_CONFIG_PINS_CMD_XOUT_MODE_LSB );

    if (L0_WriteCommandBytes(context->i2c, 6, cmdByteBuffer) != 6) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 6, rspByteBuffer, &status);
        rsp->config_pins.STATUS = &status;
        if (!error_code)  {
          rsp->config_pins.gpio1_mode  =   (( ( (rspByteBuffer[1]  )) >> Si2153_CONFIG_PINS_RESPONSE_GPIO1_MODE_LSB  ) & Si2153_CONFIG_PINS_RESPONSE_GPIO1_MODE_MASK  );
          rsp->config_pins.gpio1_state =   (( ( (rspByteBuffer[1]  )) >> Si2153_CONFIG_PINS_RESPONSE_GPIO1_STATE_LSB ) & Si2153_CONFIG_PINS_RESPONSE_GPIO1_STATE_MASK );
          rsp->config_pins.gpio2_mode  =   (( ( (rspByteBuffer[2]  )) >> Si2153_CONFIG_PINS_RESPONSE_GPIO2_MODE_LSB  ) & Si2153_CONFIG_PINS_RESPONSE_GPIO2_MODE_MASK  );
          rsp->config_pins.gpio2_state =   (( ( (rspByteBuffer[2]  )) >> Si2153_CONFIG_PINS_RESPONSE_GPIO2_STATE_LSB ) & Si2153_CONFIG_PINS_RESPONSE_GPIO2_STATE_MASK );
          rsp->config_pins.gpio3_mode  =   (( ( (rspByteBuffer[3]  )) >> Si2153_CONFIG_PINS_RESPONSE_GPIO3_MODE_LSB  ) & Si2153_CONFIG_PINS_RESPONSE_GPIO3_MODE_MASK  );
          rsp->config_pins.gpio3_state =   (( ( (rspByteBuffer[3]  )) >> Si2153_CONFIG_PINS_RESPONSE_GPIO3_STATE_LSB ) & Si2153_CONFIG_PINS_RESPONSE_GPIO3_STATE_MASK );
          rsp->config_pins.bclk1_mode  =   (( ( (rspByteBuffer[4]  )) >> Si2153_CONFIG_PINS_RESPONSE_BCLK1_MODE_LSB  ) & Si2153_CONFIG_PINS_RESPONSE_BCLK1_MODE_MASK  );
          rsp->config_pins.bclk1_state =   (( ( (rspByteBuffer[4]  )) >> Si2153_CONFIG_PINS_RESPONSE_BCLK1_STATE_LSB ) & Si2153_CONFIG_PINS_RESPONSE_BCLK1_STATE_MASK );
          rsp->config_pins.xout_mode   =   (( ( (rspByteBuffer[5]  )) >> Si2153_CONFIG_PINS_RESPONSE_XOUT_MODE_LSB   ) & Si2153_CONFIG_PINS_RESPONSE_XOUT_MODE_MASK   );
        }
    }
  exit:
    return error_code;
}
#endif /* Si2153_CONFIG_PINS_CMD */
#ifdef    Si2153_DOWNLOAD_DATASET_CONTINUE_CMD
 /*---------------------------------------------------*/
/* Si2153_DOWNLOAD_DATASET_CONTINUE COMMAND        */
/*---------------------------------------------------*/
unsigned char Si2153_L1_DOWNLOAD_DATASET_CONTINUE (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   data0,
                                                                                      unsigned char   data1,
                                                                                      unsigned char   data2,
                                                                                      unsigned char   data3,
                                                                                      unsigned char   data4,
                                                                                      unsigned char   data5,
                                                                                      unsigned char   data6,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[8];
    unsigned char rspByteBuffer[1];

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_DOWNLOAD_DATASET_CONTINUE_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( data0 & Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA0_MASK ) << Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA0_LSB);
    cmdByteBuffer[2] = (unsigned char) ( ( data1 & Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA1_MASK ) << Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA1_LSB);
    cmdByteBuffer[3] = (unsigned char) ( ( data2 & Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA2_MASK ) << Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA2_LSB);
    cmdByteBuffer[4] = (unsigned char) ( ( data3 & Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA3_MASK ) << Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA3_LSB);
    cmdByteBuffer[5] = (unsigned char) ( ( data4 & Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA4_MASK ) << Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA4_LSB);
    cmdByteBuffer[6] = (unsigned char) ( ( data5 & Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA5_MASK ) << Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA5_LSB);
    cmdByteBuffer[7] = (unsigned char) ( ( data6 & Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA6_MASK ) << Si2153_DOWNLOAD_DATASET_CONTINUE_CMD_DATA6_LSB);

    if (L0_WriteCommandBytes(context->i2c, 8, cmdByteBuffer) != 8) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 1, rspByteBuffer, &status);
        rsp->download_dataset_continue.STATUS = &status;
    }
  exit:
    return error_code;
}
#endif /* Si2153_DOWNLOAD_DATASET_CONTINUE_CMD */
#ifdef    Si2153_DOWNLOAD_DATASET_START_CMD
 /*---------------------------------------------------*/
/* Si2153_DOWNLOAD_DATASET_START COMMAND           */
/*---------------------------------------------------*/
unsigned char Si2153_L1_DOWNLOAD_DATASET_START    (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   dataset_id,
                                                                                      unsigned char   dataset_checksum,
                                                                                      unsigned char   data0,
                                                                                      unsigned char   data1,
                                                                                      unsigned char   data2,
                                                                                      unsigned char   data3,
                                                                                      unsigned char   data4,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[8];
    unsigned char rspByteBuffer[1];

  #ifdef   DEBUG_RANGE_CHECK
    if ( (dataset_id       > Si2153_DOWNLOAD_DATASET_START_CMD_DATASET_ID_MAX      )  || (dataset_id       < Si2153_DOWNLOAD_DATASET_START_CMD_DATASET_ID_MIN      ) )
    return ERROR_Si2153_PARAMETER_OUT_OF_RANGE;
  #endif /* DEBUG_RANGE_CHECK */

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_DOWNLOAD_DATASET_START_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( dataset_id       & Si2153_DOWNLOAD_DATASET_START_CMD_DATASET_ID_MASK       ) << Si2153_DOWNLOAD_DATASET_START_CMD_DATASET_ID_LSB      );
    cmdByteBuffer[2] = (unsigned char) ( ( dataset_checksum & Si2153_DOWNLOAD_DATASET_START_CMD_DATASET_CHECKSUM_MASK ) << Si2153_DOWNLOAD_DATASET_START_CMD_DATASET_CHECKSUM_LSB);
    cmdByteBuffer[3] = (unsigned char) ( ( data0            & Si2153_DOWNLOAD_DATASET_START_CMD_DATA0_MASK            ) << Si2153_DOWNLOAD_DATASET_START_CMD_DATA0_LSB           );
    cmdByteBuffer[4] = (unsigned char) ( ( data1            & Si2153_DOWNLOAD_DATASET_START_CMD_DATA1_MASK            ) << Si2153_DOWNLOAD_DATASET_START_CMD_DATA1_LSB           );
    cmdByteBuffer[5] = (unsigned char) ( ( data2            & Si2153_DOWNLOAD_DATASET_START_CMD_DATA2_MASK            ) << Si2153_DOWNLOAD_DATASET_START_CMD_DATA2_LSB           );
    cmdByteBuffer[6] = (unsigned char) ( ( data3            & Si2153_DOWNLOAD_DATASET_START_CMD_DATA3_MASK            ) << Si2153_DOWNLOAD_DATASET_START_CMD_DATA3_LSB           );
    cmdByteBuffer[7] = (unsigned char) ( ( data4            & Si2153_DOWNLOAD_DATASET_START_CMD_DATA4_MASK            ) << Si2153_DOWNLOAD_DATASET_START_CMD_DATA4_LSB           );

    if (L0_WriteCommandBytes(context->i2c, 8, cmdByteBuffer) != 8) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 1, rspByteBuffer, &status);
        rsp->download_dataset_start.STATUS = &status;
    }
  exit:
    return error_code;
}
#endif /* Si2153_DOWNLOAD_DATASET_START_CMD */
#ifdef    Si2153_DTV_RESTART_CMD
 /*---------------------------------------------------*/
/* Si2153_DTV_RESTART COMMAND                      */
/*---------------------------------------------------*/
unsigned char Si2153_L1_DTV_RESTART               (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[1];
    unsigned char rspByteBuffer[1];

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_DTV_RESTART_CMD;

    if (L0_WriteCommandBytes(context->i2c, 1, cmdByteBuffer) != 1) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 1, rspByteBuffer, &status);
        rsp->dtv_restart.STATUS = &status;
    }
  exit:
    return error_code;
}
#endif /* Si2153_DTV_RESTART_CMD */
#ifdef    Si2153_DTV_STATUS_CMD
 /*---------------------------------------------------*/
/* Si2153_DTV_STATUS COMMAND                       */
/*---------------------------------------------------*/
unsigned char Si2153_L1_DTV_STATUS                (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   intack,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[2];
    unsigned char rspByteBuffer[4];

  #ifdef   DEBUG_RANGE_CHECK
    if ( (intack > Si2153_DTV_STATUS_CMD_INTACK_MAX) )
    return ERROR_Si2153_PARAMETER_OUT_OF_RANGE;
  #endif /* DEBUG_RANGE_CHECK */

     if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_DTV_STATUS_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( intack & Si2153_DTV_STATUS_CMD_INTACK_MASK ) << Si2153_DTV_STATUS_CMD_INTACK_LSB);

    if (L0_WriteCommandBytes(context->i2c, 2, cmdByteBuffer) != 2) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 4, rspByteBuffer, &status);
        rsp->dtv_status.STATUS = &status;
        if (!error_code)  {
          rsp->dtv_status.chlint     =   (( ( (rspByteBuffer[1]  )) >> Si2153_DTV_STATUS_RESPONSE_CHLINT_LSB     ) & Si2153_DTV_STATUS_RESPONSE_CHLINT_MASK     );
          rsp->dtv_status.chl        =   (( ( (rspByteBuffer[2]  )) >> Si2153_DTV_STATUS_RESPONSE_CHL_LSB        ) & Si2153_DTV_STATUS_RESPONSE_CHL_MASK        );
          rsp->dtv_status.bw         =   (( ( (rspByteBuffer[3]  )) >> Si2153_DTV_STATUS_RESPONSE_BW_LSB         ) & Si2153_DTV_STATUS_RESPONSE_BW_MASK         );
          rsp->dtv_status.modulation =   (( ( (rspByteBuffer[3]  )) >> Si2153_DTV_STATUS_RESPONSE_MODULATION_LSB ) & Si2153_DTV_STATUS_RESPONSE_MODULATION_MASK );
        }
    }
  exit:
    return error_code;
}
#endif /* Si2153_DTV_STATUS_CMD */
#ifdef    Si2153_EXIT_BOOTLOADER_CMD
 /*---------------------------------------------------*/
/* Si2153_EXIT_BOOTLOADER COMMAND                  */
/*---------------------------------------------------*/
unsigned char Si2153_L1_EXIT_BOOTLOADER           (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   func,
                                                                                      unsigned char   ctsien,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[2];
    unsigned char rspByteBuffer[1];

  #ifdef   DEBUG_RANGE_CHECK
    if ( (func   > Si2153_EXIT_BOOTLOADER_CMD_FUNC_MAX  )
      || (ctsien > Si2153_EXIT_BOOTLOADER_CMD_CTSIEN_MAX) )
    return ERROR_Si2153_PARAMETER_OUT_OF_RANGE;
  #endif /* DEBUG_RANGE_CHECK */

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_EXIT_BOOTLOADER_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( func   & Si2153_EXIT_BOOTLOADER_CMD_FUNC_MASK   ) << Si2153_EXIT_BOOTLOADER_CMD_FUNC_LSB  |
                          ( ctsien & Si2153_EXIT_BOOTLOADER_CMD_CTSIEN_MASK ) << Si2153_EXIT_BOOTLOADER_CMD_CTSIEN_LSB);

    if (L0_WriteCommandBytes(context->i2c, 2, cmdByteBuffer) != 2) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 1, rspByteBuffer, &status);
        rsp->exit_bootloader.STATUS = &status;
    }
  exit:
    return error_code;
}
#endif /* Si2153_EXIT_BOOTLOADER_CMD */
#ifdef    Si2153_FINE_TUNE_CMD
 /*---------------------------------------------------*/
/* Si2153_FINE_TUNE COMMAND                        */
/*---------------------------------------------------*/
unsigned char Si2153_L1_FINE_TUNE                 (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   reserved,
                                                                                      int             offset_500hz,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[4];
    unsigned char rspByteBuffer[1];

  #ifdef   DEBUG_RANGE_CHECK
    if ( (reserved     > Si2153_FINE_TUNE_CMD_RESERVED_MAX    )
      || (offset_500hz > Si2153_FINE_TUNE_CMD_OFFSET_500HZ_MAX)  || (offset_500hz < Si2153_FINE_TUNE_CMD_OFFSET_500HZ_MIN) )
    return ERROR_Si2153_PARAMETER_OUT_OF_RANGE;
  #endif /* DEBUG_RANGE_CHECK */

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_FINE_TUNE_CMD;
	cmdByteBuffer[1] = (unsigned char) ( ( reserved     & Si2153_FINE_TUNE_CMD_RESERVED_MASK     ) << Si2153_FINE_TUNE_CMD_RESERVED_LSB    );
    cmdByteBuffer[2] = (unsigned char) ( ( offset_500hz & Si2153_FINE_TUNE_CMD_OFFSET_500HZ_MASK ) << Si2153_FINE_TUNE_CMD_OFFSET_500HZ_LSB);
    cmdByteBuffer[3] = (unsigned char) ((( offset_500hz & Si2153_FINE_TUNE_CMD_OFFSET_500HZ_MASK ) << Si2153_FINE_TUNE_CMD_OFFSET_500HZ_LSB)>>8);

    if (L0_WriteCommandBytes(context->i2c, 4, cmdByteBuffer) != 4) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 1, rspByteBuffer, &status);
        rsp->fine_tune.STATUS = &status;
    }
  exit:
    return error_code;
}
#endif /* Si2153_FINE_TUNE_CMD */

#ifdef    Si2153_GET_PROPERTY_CMD
 /*---------------------------------------------------*/
/* Si2153_GET_PROPERTY COMMAND                     */
/*---------------------------------------------------*/
unsigned char Si2153_L1_GET_PROPERTY              (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   reserved,
                                                                                      unsigned int    prop,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[4];
    unsigned char rspByteBuffer[4];

  #ifdef   DEBUG_RANGE_CHECK
    if ( (reserved > Si2153_GET_PROPERTY_CMD_RESERVED_MAX) )
    return ERROR_Si2153_PARAMETER_OUT_OF_RANGE;
  #endif /* DEBUG_RANGE_CHECK */

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_GET_PROPERTY_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( reserved & Si2153_GET_PROPERTY_CMD_RESERVED_MASK ) << Si2153_GET_PROPERTY_CMD_RESERVED_LSB);
    cmdByteBuffer[2] = (unsigned char) ( ( prop     & Si2153_GET_PROPERTY_CMD_PROP_MASK     ) << Si2153_GET_PROPERTY_CMD_PROP_LSB    );
    cmdByteBuffer[3] = (unsigned char) ((( prop     & Si2153_GET_PROPERTY_CMD_PROP_MASK     ) << Si2153_GET_PROPERTY_CMD_PROP_LSB    )>>8);

    if (L0_WriteCommandBytes(context->i2c, 4, cmdByteBuffer) != 4) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 4, rspByteBuffer, &status);
        rsp->get_property.STATUS = &status;
        if (!error_code)  {
          rsp->get_property.reserved =   (( ( (rspByteBuffer[1]  )) >> Si2153_GET_PROPERTY_RESPONSE_RESERVED_LSB ) & Si2153_GET_PROPERTY_RESPONSE_RESERVED_MASK );
          rsp->get_property.data     =   (( ( (rspByteBuffer[2]  ) | (rspByteBuffer[3]  << 8 )) >> Si2153_GET_PROPERTY_RESPONSE_DATA_LSB     ) & Si2153_GET_PROPERTY_RESPONSE_DATA_MASK     );
        }
    }
  exit:
    return error_code;
}
#endif /* Si2153_GET_PROPERTY_CMD */
#ifdef    Si2153_GET_REV_CMD
 /*---------------------------------------------------*/
/* Si2153_GET_REV COMMAND                          */
/*---------------------------------------------------*/
unsigned char Si2153_L1_GET_REV                   (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[1];
    unsigned char rspByteBuffer[10];

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_GET_REV_CMD;

    if (L0_WriteCommandBytes(context->i2c, 1, cmdByteBuffer) != 1) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 10, rspByteBuffer, &status);
        rsp->get_rev.STATUS = &status;
        if (!error_code)  {
          rsp->get_rev.pn       =   (( ( (rspByteBuffer[1]  )) >> Si2153_GET_REV_RESPONSE_PN_LSB       ) & Si2153_GET_REV_RESPONSE_PN_MASK       );
          rsp->get_rev.fwmajor  =   (( ( (rspByteBuffer[2]  )) >> Si2153_GET_REV_RESPONSE_FWMAJOR_LSB  ) & Si2153_GET_REV_RESPONSE_FWMAJOR_MASK  );
          rsp->get_rev.fwminor  =   (( ( (rspByteBuffer[3]  )) >> Si2153_GET_REV_RESPONSE_FWMINOR_LSB  ) & Si2153_GET_REV_RESPONSE_FWMINOR_MASK  );
          rsp->get_rev.patch    =   (( ( (rspByteBuffer[4]  ) | (rspByteBuffer[5]  << 8 )) >> Si2153_GET_REV_RESPONSE_PATCH_LSB    ) & Si2153_GET_REV_RESPONSE_PATCH_MASK    );
          rsp->get_rev.cmpmajor =   (( ( (rspByteBuffer[6]  )) >> Si2153_GET_REV_RESPONSE_CMPMAJOR_LSB ) & Si2153_GET_REV_RESPONSE_CMPMAJOR_MASK );
          rsp->get_rev.cmpminor =   (( ( (rspByteBuffer[7]  )) >> Si2153_GET_REV_RESPONSE_CMPMINOR_LSB ) & Si2153_GET_REV_RESPONSE_CMPMINOR_MASK );
          rsp->get_rev.cmpbuild =   (( ( (rspByteBuffer[8]  )) >> Si2153_GET_REV_RESPONSE_CMPBUILD_LSB ) & Si2153_GET_REV_RESPONSE_CMPBUILD_MASK );
          rsp->get_rev.chiprev  =   (( ( (rspByteBuffer[9]  )) >> Si2153_GET_REV_RESPONSE_CHIPREV_LSB  ) & Si2153_GET_REV_RESPONSE_CHIPREV_MASK  );
        }
    }
  exit:
    return error_code;
}
#endif /* Si2153_GET_REV_CMD */
#ifdef    Si2153_PART_INFO_CMD
 /*---------------------------------------------------*/
/* Si2153_PART_INFO COMMAND                        */
/*---------------------------------------------------*/
unsigned char Si2153_L1_PART_INFO                 (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[1];
    unsigned char rspByteBuffer[13];

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_PART_INFO_CMD;

    if (L0_WriteCommandBytes(context->i2c, 1, cmdByteBuffer) != 1) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 13, rspByteBuffer, &status);
        rsp->part_info.STATUS = &status;
        if (!error_code)  {
          rsp->part_info.chiprev  =   (( ( (rspByteBuffer[1]  )) >> Si2153_PART_INFO_RESPONSE_CHIPREV_LSB  ) & Si2153_PART_INFO_RESPONSE_CHIPREV_MASK  );
          rsp->part_info.part     =   (( ( (rspByteBuffer[2]  )) >> Si2153_PART_INFO_RESPONSE_PART_LSB     ) & Si2153_PART_INFO_RESPONSE_PART_MASK     );
          rsp->part_info.pmajor   =   (( ( (rspByteBuffer[3]  )) >> Si2153_PART_INFO_RESPONSE_PMAJOR_LSB   ) & Si2153_PART_INFO_RESPONSE_PMAJOR_MASK   );
          rsp->part_info.pminor   =   (( ( (rspByteBuffer[4]  )) >> Si2153_PART_INFO_RESPONSE_PMINOR_LSB   ) & Si2153_PART_INFO_RESPONSE_PMINOR_MASK   );
          rsp->part_info.pbuild   =   (( ( (rspByteBuffer[5]  )) >> Si2153_PART_INFO_RESPONSE_PBUILD_LSB   ) & Si2153_PART_INFO_RESPONSE_PBUILD_MASK   );
          rsp->part_info.reserved =   (( ( (rspByteBuffer[6]  ) | (rspByteBuffer[7]  << 8 )) >> Si2153_PART_INFO_RESPONSE_RESERVED_LSB ) & Si2153_PART_INFO_RESPONSE_RESERVED_MASK );
          rsp->part_info.serial   =   (( ( (rspByteBuffer[8]  ) | (rspByteBuffer[9]  << 8 ) | (rspByteBuffer[10] << 16 ) | (rspByteBuffer[11] << 24 )) >> Si2153_PART_INFO_RESPONSE_SERIAL_LSB   ) & Si2153_PART_INFO_RESPONSE_SERIAL_MASK   );
          rsp->part_info.romid    =   (( ( (rspByteBuffer[12] )) >> Si2153_PART_INFO_RESPONSE_ROMID_LSB    ) & Si2153_PART_INFO_RESPONSE_ROMID_MASK    );
        }
    }
  exit:
    return error_code;
}
#endif /* Si2153_PART_INFO_CMD */
#ifdef    Si2153_POWER_DOWN_CMD
 /*---------------------------------------------------*/
/* Si2153_POWER_DOWN COMMAND                       */
/*---------------------------------------------------*/
unsigned char Si2153_L1_POWER_DOWN                (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[1];
    unsigned char rspByteBuffer[1];

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_POWER_DOWN_CMD;

    if (L0_WriteCommandBytes(context->i2c, 1, cmdByteBuffer) != 1) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 1, rspByteBuffer, &status);
        rsp->power_down.STATUS = &status;
    }
  exit:
    return error_code;
}
#endif /* Si2153_POWER_DOWN_CMD */
#ifdef    Si2153_POWER_UP_CMD
 /*---------------------------------------------------*/
/* Si2153_POWER_UP COMMAND                         */
/*---------------------------------------------------*/
unsigned char Si2153_L1_POWER_UP                  (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   subcode,
                                                                                      unsigned char   reserved1,
                                                                                      unsigned char   reserved2,
                                                                                      unsigned char   reserved3,
                                                                                      unsigned char   clock_mode,
                                                                                      unsigned char   clock_freq,
                                                                                      unsigned char   addr_mode,
                                                                                      unsigned char   func,
                                                                                      unsigned char   ctsien,
                                                                                      unsigned char   wake_up,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[9];
    unsigned char rspByteBuffer[1];

 #ifdef   DEBUG_RANGE_CHECK
    if ( (subcode    > Si2153_POWER_UP_CMD_SUBCODE_MAX   )  || (subcode    < Si2153_POWER_UP_CMD_SUBCODE_MIN   )
      || (reserved1  > Si2153_POWER_UP_CMD_RESERVED1_MAX )  || (reserved1  < Si2153_POWER_UP_CMD_RESERVED1_MIN )
      || (reserved2  > Si2153_POWER_UP_CMD_RESERVED2_MAX )
      || (reserved3  > Si2153_POWER_UP_CMD_RESERVED3_MAX )
      || (clock_mode > Si2153_POWER_UP_CMD_CLOCK_MODE_MAX)  || (clock_mode < Si2153_POWER_UP_CMD_CLOCK_MODE_MIN)
      || (clock_freq > Si2153_POWER_UP_CMD_CLOCK_FREQ_MAX)
      || (addr_mode  > Si2153_POWER_UP_CMD_ADDR_MODE_MAX )
      || (func       > Si2153_POWER_UP_CMD_FUNC_MAX      )
      || (ctsien     > Si2153_POWER_UP_CMD_CTSIEN_MAX    )
      || (wake_up    > Si2153_POWER_UP_CMD_WAKE_UP_MAX   )  || (wake_up    < Si2153_POWER_UP_CMD_WAKE_UP_MIN   ) )
    return ERROR_Si2153_PARAMETER_OUT_OF_RANGE;
  #endif /* DEBUG_RANGE_CHECK */

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_POWER_UP_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( subcode    & Si2153_POWER_UP_CMD_SUBCODE_MASK    ) << Si2153_POWER_UP_CMD_SUBCODE_LSB   );
    cmdByteBuffer[2] = (unsigned char) ( ( reserved1  & Si2153_POWER_UP_CMD_RESERVED1_MASK  ) << Si2153_POWER_UP_CMD_RESERVED1_LSB );
    cmdByteBuffer[3] = (unsigned char) ( ( reserved2  & Si2153_POWER_UP_CMD_RESERVED2_MASK  ) << Si2153_POWER_UP_CMD_RESERVED2_LSB );
    cmdByteBuffer[4] = (unsigned char) ( ( reserved3  & Si2153_POWER_UP_CMD_RESERVED3_MASK  ) << Si2153_POWER_UP_CMD_RESERVED3_LSB );
    cmdByteBuffer[5] = (unsigned char) ( ( clock_mode & Si2153_POWER_UP_CMD_CLOCK_MODE_MASK ) << Si2153_POWER_UP_CMD_CLOCK_MODE_LSB|
                          ( clock_freq & Si2153_POWER_UP_CMD_CLOCK_FREQ_MASK ) << Si2153_POWER_UP_CMD_CLOCK_FREQ_LSB);
    cmdByteBuffer[6] = (unsigned char) ( ( addr_mode  & Si2153_POWER_UP_CMD_ADDR_MODE_MASK  ) << Si2153_POWER_UP_CMD_ADDR_MODE_LSB );
    cmdByteBuffer[7] = (unsigned char) ( ( func       & Si2153_POWER_UP_CMD_FUNC_MASK       ) << Si2153_POWER_UP_CMD_FUNC_LSB      |
                          ( ctsien     & Si2153_POWER_UP_CMD_CTSIEN_MASK     ) << Si2153_POWER_UP_CMD_CTSIEN_LSB    );
    cmdByteBuffer[8] = (unsigned char) ( ( wake_up    & Si2153_POWER_UP_CMD_WAKE_UP_MASK    ) << Si2153_POWER_UP_CMD_WAKE_UP_LSB   );

    if (L0_WriteCommandBytes(context->i2c, 9, cmdByteBuffer) != 9) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 1, rspByteBuffer, &status);
        rsp->power_up.STATUS = &status;
    }
  exit:
    return error_code;
}
#endif /* Si2153_POWER_UP_CMD */
#ifdef    Si2153_SET_PROPERTY_CMD
 /*---------------------------------------------------*/
/* Si2153_SET_PROPERTY COMMAND                     */
/*---------------------------------------------------*/
unsigned char Si2153_L1_SET_PROPERTY              (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   reserved,
                                                                                      unsigned int    prop,
                                                                                      unsigned int    data,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[6];
    unsigned char rspByteBuffer[4];

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_SET_PROPERTY_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( reserved & Si2153_SET_PROPERTY_CMD_RESERVED_MASK ) << Si2153_SET_PROPERTY_CMD_RESERVED_LSB);
    cmdByteBuffer[2] = (unsigned char) ( ( prop     & Si2153_SET_PROPERTY_CMD_PROP_MASK     ) << Si2153_SET_PROPERTY_CMD_PROP_LSB    );
    cmdByteBuffer[3] = (unsigned char) ((( prop     & Si2153_SET_PROPERTY_CMD_PROP_MASK     ) << Si2153_SET_PROPERTY_CMD_PROP_LSB    )>>8);
    cmdByteBuffer[4] = (unsigned char) ( ( data     & Si2153_SET_PROPERTY_CMD_DATA_MASK     ) << Si2153_SET_PROPERTY_CMD_DATA_LSB    );
    cmdByteBuffer[5] = (unsigned char) ((( data     & Si2153_SET_PROPERTY_CMD_DATA_MASK     ) << Si2153_SET_PROPERTY_CMD_DATA_LSB    )>>8);

    if (L0_WriteCommandBytes(context->i2c, 6, cmdByteBuffer) != 6) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 4, rspByteBuffer, &status);
        rsp->set_property.STATUS = &status;
        if (!error_code)  {
          rsp->set_property.reserved =   (( ( (rspByteBuffer[1]  )) >> Si2153_SET_PROPERTY_RESPONSE_RESERVED_LSB ) & Si2153_SET_PROPERTY_RESPONSE_RESERVED_MASK );
          rsp->set_property.data     =   (( ( (rspByteBuffer[2]  ) | (rspByteBuffer[3]  << 8 )) >> Si2153_SET_PROPERTY_RESPONSE_DATA_LSB     ) & Si2153_SET_PROPERTY_RESPONSE_DATA_MASK     );
        }
    }
  exit:
    return error_code;
}
#endif /* Si2153_SET_PROPERTY_CMD */
#ifdef    Si2153_STANDBY_CMD
 /*---------------------------------------------------*/
/* Si2153_STANDBY COMMAND                          */
/*---------------------------------------------------*/
unsigned char Si2153_L1_STANDBY                   (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[1];
    unsigned char rspByteBuffer[1];

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_STANDBY_CMD;

    if (L0_WriteCommandBytes(context->i2c, 1, cmdByteBuffer) != 1) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 1, rspByteBuffer, &status);
        rsp->standby.STATUS = &status;
    }
  exit:
    return error_code;
}
#endif /* Si2153_STANDBY_CMD */
#ifdef    Si2153_TUNER_STATUS_CMD
 /*---------------------------------------------------*/
/* Si2153_TUNER_STATUS COMMAND                     */
/*---------------------------------------------------*/
 unsigned char  Si2153_L1_TUNER_STATUS              (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   intack,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[2];
    unsigned char rspByteBuffer[12];

 #ifdef   DEBUG_RANGE_CHECK
    if ( (intack > Si2153_TUNER_STATUS_CMD_INTACK_MAX) )
    return ERROR_Si2153_PARAMETER_OUT_OF_RANGE;
 #endif /* DEBUG_RANGE_CHECK */

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_TUNER_STATUS_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( intack & Si2153_TUNER_STATUS_CMD_INTACK_MASK ) << Si2153_TUNER_STATUS_CMD_INTACK_LSB);

    if (L0_WriteCommandBytes(context->i2c, 2, cmdByteBuffer) != 2) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 12, rspByteBuffer, &status);
        rsp->tuner_status.STATUS = &status;
        if (!error_code)  {
          rsp->tuner_status.tcint    =   (( ( (rspByteBuffer[1]  )) >> Si2153_TUNER_STATUS_RESPONSE_TCINT_LSB    ) & Si2153_TUNER_STATUS_RESPONSE_TCINT_MASK    );
          rsp->tuner_status.rssilint =   (( ( (rspByteBuffer[1]  )) >> Si2153_TUNER_STATUS_RESPONSE_RSSILINT_LSB ) & Si2153_TUNER_STATUS_RESPONSE_RSSILINT_MASK );
          rsp->tuner_status.rssihint =   (( ( (rspByteBuffer[1]  )) >> Si2153_TUNER_STATUS_RESPONSE_RSSIHINT_LSB ) & Si2153_TUNER_STATUS_RESPONSE_RSSIHINT_MASK );
          rsp->tuner_status.tc       =   (( ( (rspByteBuffer[2]  )) >> Si2153_TUNER_STATUS_RESPONSE_TC_LSB       ) & Si2153_TUNER_STATUS_RESPONSE_TC_MASK       );
          rsp->tuner_status.rssil    =   (( ( (rspByteBuffer[2]  )) >> Si2153_TUNER_STATUS_RESPONSE_RSSIL_LSB    ) & Si2153_TUNER_STATUS_RESPONSE_RSSIL_MASK    );
          rsp->tuner_status.rssih    =   (( ( (rspByteBuffer[2]  )) >> Si2153_TUNER_STATUS_RESPONSE_RSSIH_LSB    ) & Si2153_TUNER_STATUS_RESPONSE_RSSIH_MASK    );
          rsp->tuner_status.rssi     = (((( ( (rspByteBuffer[3]  )) >> Si2153_TUNER_STATUS_RESPONSE_RSSI_LSB     ) & Si2153_TUNER_STATUS_RESPONSE_RSSI_MASK) <<Si2153_TUNER_STATUS_RESPONSE_RSSI_SHIFT ) >>Si2153_TUNER_STATUS_RESPONSE_RSSI_SHIFT     );
          rsp->tuner_status.freq     =   (( ( (rspByteBuffer[4]  ) | (rspByteBuffer[5]  << 8 ) | (rspByteBuffer[6]  << 16 ) | (rspByteBuffer[7]  << 24 )) >> Si2153_TUNER_STATUS_RESPONSE_FREQ_LSB     ) & Si2153_TUNER_STATUS_RESPONSE_FREQ_MASK     );
          rsp->tuner_status.mode     =   (( ( (rspByteBuffer[8]  )) >> Si2153_TUNER_STATUS_RESPONSE_MODE_LSB     ) & Si2153_TUNER_STATUS_RESPONSE_MODE_MASK     );
          rsp->tuner_status.vco_code = (((( ( (rspByteBuffer[10] ) | (rspByteBuffer[11] << 8 )) >> Si2153_TUNER_STATUS_RESPONSE_VCO_CODE_LSB ) & Si2153_TUNER_STATUS_RESPONSE_VCO_CODE_MASK) <<Si2153_TUNER_STATUS_RESPONSE_VCO_CODE_SHIFT ) >>Si2153_TUNER_STATUS_RESPONSE_VCO_CODE_SHIFT );
        }
    }
  exit:
    return error_code;
}
#endif /* Si2153_TUNER_STATUS_CMD */
#ifdef    Si2153_TUNER_TUNE_FREQ_CMD
 /*---------------------------------------------------*/
/* Si2153_TUNER_TUNE_FREQ COMMAND                  */
/*---------------------------------------------------*/
 unsigned char  Si2153_L1_TUNER_TUNE_FREQ           (L1_Si2153_Context *context, unsigned char Si2153_waitForCTS, unsigned char Si2153_waitForResponse,
                                                                                      unsigned char   mode,
                                                                                      unsigned long   freq,
                                                                                      Si2153_CmdReplyObj *rsp)
{
    unsigned char error_code = 0;
    unsigned char cmdByteBuffer[8];
    unsigned char rspByteBuffer[1];

  #ifdef   DEBUG_RANGE_CHECK
    if ( (mode > Si2153_TUNER_TUNE_FREQ_CMD_MODE_MAX)
      || (freq > Si2153_TUNER_TUNE_FREQ_CMD_FREQ_MAX)  || (freq < Si2153_TUNER_TUNE_FREQ_CMD_FREQ_MIN) )
    return ERROR_Si2153_PARAMETER_OUT_OF_RANGE;
  #endif /* DEBUG_RANGE_CHECK */

    if (Si2153_waitForCTS) {
        error_code = Si2153_pollForCTS(context, Si2153_waitForCTS);
        if (error_code) goto exit;
    }

    cmdByteBuffer[0] = Si2153_TUNER_TUNE_FREQ_CMD;
    cmdByteBuffer[1] = (unsigned char) ( ( mode & Si2153_TUNER_TUNE_FREQ_CMD_MODE_MASK ) << Si2153_TUNER_TUNE_FREQ_CMD_MODE_LSB);
    cmdByteBuffer[2] = (unsigned char)0x00;
    cmdByteBuffer[3] = (unsigned char)0x00;
    cmdByteBuffer[4] = (unsigned char) ( ( freq & Si2153_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << Si2153_TUNER_TUNE_FREQ_CMD_FREQ_LSB);
    cmdByteBuffer[5] = (unsigned char) ((( freq & Si2153_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << Si2153_TUNER_TUNE_FREQ_CMD_FREQ_LSB)>>8);
    cmdByteBuffer[6] = (unsigned char) ((( freq & Si2153_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << Si2153_TUNER_TUNE_FREQ_CMD_FREQ_LSB)>>16);
    cmdByteBuffer[7] = (unsigned char) ((( freq & Si2153_TUNER_TUNE_FREQ_CMD_FREQ_MASK ) << Si2153_TUNER_TUNE_FREQ_CMD_FREQ_LSB)>>24);

    if (L0_WriteCommandBytes(context->i2c, 8, cmdByteBuffer) != 8) error_code = ERROR_Si2153_SENDING_COMMAND;

    if (!error_code && Si2153_waitForResponse) {
        error_code = Si2153_pollForResponse(context, Si2153_waitForResponse, 1, rspByteBuffer, &status);
        rsp->tuner_tune_freq.STATUS = &status;
    }
  exit:
    return error_code;
}
#endif /* Si2153_TUNER_TUNE_FREQ_CMD */
/* _commands_insertion_point */

/* _send_command2_insertion_start */

  /* --------------------------------------------*/
  /* SEND_COMMAND2 FUNCTION                      */
  /* --------------------------------------------*/
 unsigned char    Si2153_L1_SendCommand2(L1_Si2153_Context *api, int cmd, Si2153_CmdObj *c, unsigned char wait_for_cts, unsigned char wait_for_response, Si2153_CmdReplyObj *rsp) {
    switch (cmd) {
    #ifdef        Si2153_AGC_OVERRIDE_CMD
     case         Si2153_AGC_OVERRIDE_CMD:
       return Si2153_L1_AGC_OVERRIDE (api, wait_for_cts, wait_for_response, c->agc_override.force_max_gain, c->agc_override.force_top_gain, rsp);
     break;
    #endif /*     Si2153_AGC_OVERRIDE_CMD */
    #ifdef		  Si2153_ATV_CW_TEST_CMD
     case         Si2153_ATV_CW_TEST_CMD:
       return Si2153_L1_ATV_CW_TEST (api, wait_for_cts, wait_for_response, c->atv_cw_test.pc_lock, rsp);
     break;
    #endif /*     Si2153_ATV_CW_TEST_CMD */
    #ifdef        Si2153_ATV_RESTART_CMD
     case          Si2153_ATV_RESTART_CMD:
       return Si2153_L1_ATV_RESTART (api, wait_for_cts, wait_for_response, rsp);
     break;
    #endif /*     Si2153_ATV_RESTART_CMD */
    #ifdef        Si2153_ATV_STATUS_CMD
     case         Si2153_ATV_STATUS_CMD:
       return Si2153_L1_ATV_STATUS (api, wait_for_cts, wait_for_response, c->atv_status.intack, rsp);
     break;
    #endif /*     Si2153_ATV_STATUS_CMD */
    #ifdef        Si2153_CONFIG_PINS_CMD
     case         Si2153_CONFIG_PINS_CMD:
       return Si2153_L1_CONFIG_PINS (api, wait_for_cts, wait_for_response, c->config_pins.gpio1_mode, c->config_pins.gpio1_read, c->config_pins.gpio2_mode, c->config_pins.gpio2_read, c->config_pins.gpio3_mode, c->config_pins.gpio3_read, c->config_pins.bclk1_mode, c->config_pins.bclk1_read, c->config_pins.xout_mode, rsp);
     break;
    #endif /*     Si2153_CONFIG_PINS_CMD */
    #ifdef        Si2153_DOWNLOAD_DATASET_CONTINUE_CMD
     case         Si2153_DOWNLOAD_DATASET_CONTINUE_CMD:
       return Si2153_L1_DOWNLOAD_DATASET_CONTINUE (api, wait_for_cts, wait_for_response, c->download_dataset_continue.data0, c->download_dataset_continue.data1, c->download_dataset_continue.data2, c->download_dataset_continue.data3, c->download_dataset_continue.data4, c->download_dataset_continue.data5, c->download_dataset_continue.data6, rsp);
     break;
    #endif /*     Si2153_DOWNLOAD_DATASET_CONTINUE_CMD */
    #ifdef        Si2153_DOWNLOAD_DATASET_START_CMD
     case         Si2153_DOWNLOAD_DATASET_START_CMD:
       return Si2153_L1_DOWNLOAD_DATASET_START (api, wait_for_cts, wait_for_response, c->download_dataset_start.dataset_id, c->download_dataset_start.dataset_checksum, c->download_dataset_start.data0, c->download_dataset_start.data1, c->download_dataset_start.data2, c->download_dataset_start.data3, c->download_dataset_start.data4, rsp);
     break;
    #endif /*     Si2153_DOWNLOAD_DATASET_START_CMD */
    #ifdef        Si2153_DTV_RESTART_CMD
     case         Si2153_DTV_RESTART_CMD:
       return Si2153_L1_DTV_RESTART (api, wait_for_cts, wait_for_response, rsp);
     break;
    #endif /*     Si2153_DTV_RESTART_CMD */
    #ifdef        Si2153_DTV_STATUS_CMD
     case         Si2153_DTV_STATUS_CMD:
       return Si2153_L1_DTV_STATUS (api, wait_for_cts, wait_for_response, c->dtv_status.intack, rsp);
     break;
    #endif /*     Si2153_DTV_STATUS_CMD */
    #ifdef        Si2153_EXIT_BOOTLOADER_CMD
     case         Si2153_EXIT_BOOTLOADER_CMD:
       return Si2153_L1_EXIT_BOOTLOADER (api, wait_for_cts, wait_for_response, c->exit_bootloader.func, c->exit_bootloader.ctsien, rsp);
     break;
    #endif /*     Si2153_EXIT_BOOTLOADER_CMD */
    #ifdef        Si2153_FINE_TUNE_CMD
     case         Si2153_FINE_TUNE_CMD:
       return Si2153_L1_FINE_TUNE (api, wait_for_cts, wait_for_response,c->fine_tune.reserved, c->fine_tune.offset_500hz, rsp);
     break;
    #endif /*     Si2153_FINE_TUNE_CMD */
    #ifdef        Si2153_GET_PROPERTY_CMD
     case         Si2153_GET_PROPERTY_CMD:
       return Si2153_L1_GET_PROPERTY (api, wait_for_cts, wait_for_response, c->get_property.reserved, c->get_property.prop, rsp);
     break;
    #endif /*     Si2153_GET_PROPERTY_CMD */
    #ifdef        Si2153_GET_REV_CMD
     case         Si2153_GET_REV_CMD:
       return Si2153_L1_GET_REV (api, wait_for_cts, wait_for_response, rsp);
     break;
    #endif /*     Si2153_GET_REV_CMD */
    #ifdef        Si2153_PART_INFO_CMD
     case         Si2153_PART_INFO_CMD:
       return Si2153_L1_PART_INFO (api, wait_for_cts, wait_for_response, rsp);
     break;
    #endif /*     Si2153_PART_INFO_CMD */
    #ifdef        Si2153_POWER_DOWN_CMD
     case         Si2153_POWER_DOWN_CMD:
       return Si2153_L1_POWER_DOWN (api, wait_for_cts, wait_for_response, rsp);
     break;
    #endif /*     Si2153_POWER_DOWN_CMD */
    #ifdef        Si2153_POWER_UP_CMD
     case         Si2153_POWER_UP_CMD:
       return Si2153_L1_POWER_UP (api, wait_for_cts, wait_for_response, c->power_up.subcode, c->power_up.reserved1, c->power_up.reserved2, c->power_up.reserved3, c->power_up.clock_mode, c->power_up.clock_freq, c->power_up.addr_mode, c->power_up.func, c->power_up.ctsien, c->power_up.wake_up, rsp);
     break;
    #endif /*     Si2153_POWER_UP_CMD */
    #ifdef        Si2153_SET_PROPERTY_CMD
     case         Si2153_SET_PROPERTY_CMD:
       return Si2153_L1_SET_PROPERTY (api, wait_for_cts, wait_for_response, c->set_property.reserved, c->set_property.prop, c->set_property.data, rsp);
     break;
    #endif /*     Si2153_SET_PROPERTY_CMD */
    #ifdef        Si2153_STANDBY_CMD
     case         Si2153_STANDBY_CMD:
       return Si2153_L1_STANDBY (api, wait_for_cts, wait_for_response, rsp);
     break;
    #endif /*     Si2153_STANDBY_CMD */
    #ifdef        Si2153_TUNER_STATUS_CMD
     case         Si2153_TUNER_STATUS_CMD:
       return Si2153_L1_TUNER_STATUS (api, wait_for_cts, wait_for_response, c->tuner_status.intack, rsp);
     break;
    #endif /*     Si2153_TUNER_STATUS_CMD */
    #ifdef        Si2153_TUNER_TUNE_FREQ_CMD
     case         Si2153_TUNER_TUNE_FREQ_CMD:
       return Si2153_L1_TUNER_TUNE_FREQ (api, wait_for_cts, wait_for_response, c->tuner_tune_freq.mode, c->tuner_tune_freq.freq, rsp);
     break;
    #endif /*     Si2153_TUNER_TUNE_FREQ_CMD */
   default : break;
    }
   	return 0;
  }




