/***************************************************************************************/
/* FILE: Generic_DTV_ATV_Demod.c                                                           */
/* PURPOSE: This file serves as a hook for the external demod to return the lock status */
/* This is called by the Tune() function.                                              */
/*                                                                                      */
/***************************************************************************************/
#include "Generic_DTV_ATV_Demod.h"
/************************************************************************************************************************
  NAME: L1_DigitalDemodLocked
  DESCRIPTION: Calls the external demodulator for DTV.  If the demod is locked on a valid signal then return 0 else 1
  Porting:    Replace with embedded system I2C commands to query the demod lock status
  Returns:    0 if demod locked,  1 otherwise
************************************************************************************************************************/
int L1_DigitalDemodLocked(void)
{
	//#warning "Not implemented!"
	/*Return a 0 if the demod is locked on to a DTV signal */
    return 0;
}
/************************************************************************************************************************
  NAME: L0_AnalogDemodLocked
  DESCRIPTION: Calls the external demodulator for ATV.  If the demod is locked on a valid signal then return 0 else 1
  Porting:    Replace with embedded system I2C commands to query the demod lock status
  Returns:    0 if demod locked,  1 otherwise
************************************************************************************************************************/
int L1_AnalogDemodLocked(void)
{
	//#warning "Not implemented!"
	/*Return a 0 if the demod is locked on to an ATV signal */
    return 0;
}
/***********************************************************************************************************/
