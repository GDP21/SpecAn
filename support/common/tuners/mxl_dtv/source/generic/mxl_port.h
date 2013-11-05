/*
** FILE NAME:   $RCSfile: mxl_port.h,v $
**
** TITLE:       MaxLinear tuner driver
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

#ifndef MXL_PORT_H
#define MXL_PORT_H

#include "scbm_api.h"

/* SCBM port */
//TBD Change to be multi-context...
extern SCBM_PORT_T MxLScbmPort;

int MxL_setupPort(int portNumber);
int MxL_shutdownPort(void);

#endif
