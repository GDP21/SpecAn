#ifndef OSCAR_AGC_MAIN_H
#define OSCAR_AGC_MAIN_H

void _OSCAR_AGC_initialise(void);
void _OSCAR_AGC_config(PHY_TUNER_STANDARD_T standard, long bandwidthHz);
void _OSCAR_AGC_initAGC(unsigned long muxID);
void _OSCAR_AGC_powerUp(unsigned long muxID);
void _OSCAR_AGC_powerDown(unsigned long muxID);
void _OSCAR_AGC_powerSave(PHY_RF_PWRSAV_T pwsav, unsigned long muxID);
void _OSCAR_AGC_setAGC(TUNER_AGCISR_HELPER_T *pAgcIsrHelper);

float _OSCAR_AGC_levelTrans(float rms);

#endif
