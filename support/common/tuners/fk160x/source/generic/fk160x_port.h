/*
** FILE NAME:   $RCSfile: fk160x_port.h,v $
**
** TITLE:       NuTune tuner driver
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

#ifndef FK160X_PORT_H
#define FK160X_PORT_H

#include "scbm_api.h"

	/*  Include the interface to the NuTune supplied code. */
#include "fk160x.h"

/* SCBM port */
//TBD Change to be multi-context...
extern SCBM_PORT_T FK_ScbmPort;

int FK_setupPort(int portNumber);
int FK_shutdownPort(void);


tmErrorCode_t 	FK_I2CRead(tmUnitSelect_t tUnit,UInt32 AddrSize, UInt8* pAddr,UInt32 ReadLen, UInt8* pData);
tmErrorCode_t 	FK_I2CWrite (tmUnitSelect_t tUnit, UInt32 AddrSize, UInt8* pAddr,UInt32 WriteLen, UInt8* pData);
tmErrorCode_t 	FK_Wait(tmUnitSelect_t tUnit, UInt32 tms);
tmErrorCode_t 	FK_Print(UInt32 level, const char* format, ...);

#endif
