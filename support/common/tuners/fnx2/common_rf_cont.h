/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/fnx2/common_rf_cont.h,v $
**
** TITLE:       Common RF Control
**
** AUTHOR:      Futurewaves, Peter Cheung
**
** DESCRIPTION: Common control functions used by RF variants
**
**
*/

#ifndef COMMON_RF_CONT_H
#define COMMON_RF_CONT_H


/*
** Function prototypes accessed by custom RF control functions
*/
void RFContolInit(unsigned int coreclock);
void RFContolMessage(unsigned char *buf, short size, int read_nwrite);

#endif

