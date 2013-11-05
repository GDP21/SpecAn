/*
** FILE NAME:   $Source: /cvs/mobileTV/support/common/tuners/msi_tuner/source/generic/msi_002.c,v $
**
** TITLE:       MSI tuner driver
**
** AUTHOR:      Ensigma
**
** DESCRIPTION: MSI 002 specific support
**
** NOTICE:      Copyright (C) 2007, Imagination Technologies Ltd.
**              This software is supplied under a licence agreement.
**              It must not be copied or distributed except under the
**              terms of that agreement.
**
*/

/* Keep these first ... */
#ifdef METAG
#define METAG_ALL_VALUES
#define METAC_ALL_VALUES
#include <metag/machine.inc>
#include <metag/metagtbi.h>
#endif

#include "msi_002.h"
#include <assert.h>

/*
** FUNCTION:    MSI_reformatMessage001To002
**
** DESCRIPTION: Reformats an MSI 001 message into the format required by MSI 002
**
** INPUTS:      msg   001 message
**              size  Pointer to the size of 001 message in bytes. This will get
**                    updated to the size of the 002 message in bytes.
**
** RETURNS:     void
**
*/
void MSI_reformatMessage001To002(unsigned char *msg, int *size)
{
    int i, sizeIn;
    unsigned long tmp;
    unsigned char regId;

    sizeIn = *size;
    assert(sizeIn <= 4);

    /* Read 001 message */
    tmp = 0;
    for (i = 0; i < sizeIn; i++)
    {
        tmp |= msg[sizeIn - i - 1] << (8*i);
    }

    /* Get register ID */
    regId = tmp & 0xf;

    /* Remove register ID */
    tmp = tmp >> 4;

    /* Write out re-formatted 002 message */
    msg[0] = regId;
    for (i = 0; i < sizeIn; i++)
    {
        msg[i + 1] = (tmp >> (8*i)) & 0xff;
    }

    /* New message is longer because of register ID */
    *size = *size + 1;
}

#ifdef TEST

#include <stdio.h>

void main(void)
{
    unsigned char buf[4];
    int i;
    int size;

    buf[2] = 0x1d;
    buf[1] = 0x32;
    buf[0] = 0x54;

    size = 3;
    MSI_reformatMessage001To002(buf, &size);

    printf("size = %d\n", size);
    for (i = 0; i < size; i++)
        printf("%x\n", buf[i]);
}

#endif
