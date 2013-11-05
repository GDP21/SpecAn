/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/msi_tuner/source/generic/msi_port.h,v $
**
** TITLE:       MSI tuner driver
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: Abstract interface for port mini driver
**
** NOTICE:      Copyright (C) 2007-2009, Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

#ifndef MSI_PORT_H
#define MSI_PORT_H

int MSI_setupPort(int dmacChannel, int portNumber);
int MSI_shutdownPort(void);
int MSI_writeMessage(unsigned char *buffer, unsigned long size);

#endif
