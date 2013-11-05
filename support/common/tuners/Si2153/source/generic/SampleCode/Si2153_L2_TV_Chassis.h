/***************************************************************************************/
/* FILE: Si2153_L2_TV_Chassis.h                                                        */
/*                                                                                     */
/*                                                                                     */
/*                                                                                     */
/***************************************************************************************/
#ifndef Si2153_L2_TV_CHASSIS_H
#define Si2153_L2_TV_CHASSIS_H
int		ProcessAPITopLevelSequence(void) ;
int     Si2153_Init(L1_Si2153_Context *Si2153);
int     Si2153_PowerUpWithPatch (L1_Si2153_Context *Si2153);
int     Si2153_LoadFirmware (L1_Si2153_Context *Si2153,unsigned char* firmwareTable,int lines);
int     Si2153_StartFirmware (L1_Si2153_Context *Si2153, Si2153_CmdReplyObj *rsp);
int     Si2153_Configure (L1_Si2153_Context *Si2153, Si2153_CmdReplyObj *rsp);
int		Si2153_LoadVideofilter (L1_Si2153_Context *Si2153,unsigned char* vidfiltTable,int lines) ;
int     Si2153_ATVTune (L1_Si2153_Context *Si2153,
			unsigned long freq,
			unsigned char video_sys,
			unsigned char trans,
			unsigned char color,
			unsigned char invert_spectrum,
			unsigned int afcRangeKHz,
			Si2153_CmdReplyObj *rsp);
int     Si2153_Tune (L1_Si2153_Context *Si2153, unsigned char mode, unsigned long freq, Si2153_CmdReplyObj *rsp );
int     Si2153_DTVTune (L1_Si2153_Context *Si2153, unsigned long freq, unsigned char bw, unsigned char modulation, unsigned char invert_spectrum, Si2153_CmdReplyObj *rsp);
int		Si2153_ATV_Channel_Scan_M  (L1_Si2153_Context *Si2153, unsigned long rangeMinHz, unsigned long rangeMaxHz, int minRSSIdBm, int maxRSSIdBm);
int		Si2153_ATV_Channel_Scan_PAL(L1_Si2153_Context *Si2153, unsigned long rangeMinHz, unsigned long rangeMaxHz, int minRSSIdBm, int maxRSSIdBm);
int     Si2153_ATVConfig (L1_Si2153_Context *Si2153 );
int     Si2153_DTVConfig (L1_Si2153_Context *Si2153 );
int		Si2153_CommonConfig (L1_Si2153_Context *Si2153);
int		Si2153_TunerConfig (L1_Si2153_Context *Si2153);
int		Si2153_SetupTunerDefaults(void);
int		Si2153_SetupCommonDefaults(void);
int		Si2153_SetupATVDefaults (void);
int		Si2153_SetupDTVDefaults (void);
int		TunerFrequencyChanged(unsigned long *frequency);
int		UpdateChannelScanFrequency(int freq,int channelsFound);
#endif
