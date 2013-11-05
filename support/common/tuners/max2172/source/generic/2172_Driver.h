/*
** FILE NAME:   $RCSfile: 2172_Driver.h,v $
**
** TITLE:       MAX2172 DAB/FM tuner driver
**
** AUTHOR:      Imagination Technologies
**
** DESCRIPTION: Implementation of a Maxim MAX2172 tuner driver
**
** NOTICE:      Copyright (C) Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

/*
	Derived from example driver code from Maxim:
  Name: 2172_Driver.h
  Copyright: Maxim IC
  Author: Paul Nichol
  Date: 5/9/2008
  Description: Header file for 2172_Driver.cpp
  Contains prototypes for functions
*/

#ifndef MAX2172_Driver_H
#define MAX2172_Driver_H

void Max2172Finv(int mode);
long Max2172GetFMCenterFreq(void);
void Max2172HighLow(int sideband);
void Max2172Init(void);
void Max2172LNA_BYP(int onoff);
void Max2172SetLO(long L_Frequency);

#endif
