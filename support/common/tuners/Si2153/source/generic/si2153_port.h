/*
** FILE NAME:   $RCSfile: si2153_port.h,v $
**
** TITLE:       SiLabs tuner driver
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

#ifndef SI2153_PORT_H
#define SI2153_PORT_H

#include "scbm_api.h"

	/*  Include the interface to the SiLabs supplied code. */
#include "si2153.h"

/* SCBM port */
//TBD Change to be multi-context...
extern SCBM_PORT_T SiScbmPort;

int Si_setupPort(int portNumber);
int Si_shutdownPort(void);

#endif
