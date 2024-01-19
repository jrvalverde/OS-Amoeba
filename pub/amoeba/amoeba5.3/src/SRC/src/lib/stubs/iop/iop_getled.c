/*	@(#)iop_getled.c	1.2	94/04/07 11:03:08 */
/*
 * Copyright 1994 Vrije Universiteit, The Netherlands.
 * For full copyright and restrictions on use see the file COPYRIGHT in the
 * top level of the Amoeba distribution.
 */

#include "amoeba.h"
#include "cmdreg.h"
#include "stderr.h"
#include "iop/iop.h"

errstat
iop_getleds(svr, leds)
capability *	svr;
int *		leds;
{
    header	hdr;
    bufsize	err;

    hdr.h_port = svr->cap_port;
    hdr.h_priv = svr->cap_priv;
    hdr.h_command = IOP_GETLEDS;
    err = trans(&hdr, NILBUF, 0, &hdr, NILBUF, 0);
    if (ERR_STATUS(err))
	return ERR_CONVERT(err);
    if (hdr.h_status == STD_OK)
	*leds = hdr.h_offset;
    return ERR_CONVERT(hdr.h_status);
}
