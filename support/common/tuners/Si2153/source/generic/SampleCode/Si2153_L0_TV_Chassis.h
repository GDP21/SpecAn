/***************************************************************************************/
/* FILE: Si2153_L0_TV_Chassis.h                                                        */
/*                                                                                     */
/*                                                                                     */
/*                                                                                     */
/***************************************************************************************/
#ifndef Si2153_L0_TV_CHASSIS_H
#define Si2153_L0_TV_CHASSIS_H

void	L0_Init                  (L0_Context *pContext);
void	system_wait              (int time_ms);
void	SendRSTb (void);
int		L0_ReadCommandBytes      (L0_Context* i2c, int iNbBytes, unsigned char *pucDataBuffer) ;
int		L0_WriteCommandBytes     (L0_Context* i2c, int iNbBytes, unsigned char *pucDataBuffer) ;
#endif
