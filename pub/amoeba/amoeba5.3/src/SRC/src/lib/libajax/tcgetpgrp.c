/*	@(#)tcgetpgrp.c	1.2	94/04/07 09:54:12 */
/*
 * Copyright 1994 Vrije Universiteit and Stichting Mathematisch Centrum,
 * The Netherlands.
 * For full copyright and restrictions on use see the file COPYRIGHT in
 * the top level of the Amoeba distribution.
 */

/* tcgetpgrp() POSIX 7.2.3
 * last modified apr 22 93 Ron Visser
 */

#include "ajax.h"
#include <sys/types.h>

pid_t
tcgetpgrp(fd)
	int fd;
{
	ERR(ENOSYS, "tcgetpgrp: not supported");
}
