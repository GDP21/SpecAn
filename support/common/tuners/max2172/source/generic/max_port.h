/*
** FILE NAME:   $RCSfile: max_port.h,v $
**
** TITLE:       Maxim tuner driver
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

#ifndef MAX_PORT_H
#define MAX_PORT_H

int MAX_enable(void);
int MAX_readMessage(unsigned char *buffer, unsigned long size);
int MAX_setupPort(int portNumber);
int MAX_shutdownPort(void);
int MAX_standby(void);
int MAX_writeMessage(unsigned char *buffer, unsigned long size);

#endif
