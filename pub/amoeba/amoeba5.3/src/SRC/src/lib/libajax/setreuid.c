/*	@(#)setreuid.c	1.2	94/04/07 09:51:07 */
/*
 * Copyright 1994 Vrije Universiteit and Stichting Mathematisch Centrum,
 * The Netherlands.
 * For full copyright and restrictions on use see the file COPYRIGHT in
 * the top level of the Amoeba distribution.
 */

/* setreuid(2) system call emulation */

#include "ajax.h"

int
setreuid(ruid, euid)
	int ruid, euid;
{
	return 0; /* Pretend it's done */
}
