/*
** FILE NAME:   $RCSfile: stv6110_port.h,v $
**
** TITLE:       Satellite tuner driver
**
** AUTHOR:      Imagination Technologies
**
** DESCRIPTION: Abstract interface for port mini driver
**
** NOTICE:      Copyright (C) Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

#ifndef STV6110_PORT_H
#define STV6110_PORT_H

#include "scbm_api.h"

/* SCBM port */
//TBD Change to be multi-context...
extern SCBM_PORT_T STV_ScbmPort;

int STV_setupPort(int portNumber);
int STV_shutdownPort(void);
int STV_writeMessage(int i2cAddress, unsigned char *buffer, unsigned long size);
int STV_readMessage(int i2cAddress, unsigned char registerAddress, unsigned char *buffer, unsigned long size);

#endif
