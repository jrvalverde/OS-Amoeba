/*	@(#)uniqport.c	1.3	96/02/27 11:56:13 */
/*
 * Copyright 1994 Vrije Universiteit, The Netherlands.
 * For full copyright and restrictions on use see the file COPYRIGHT in the
 * top level of the Amoeba distribution.
 */

#include "amoeba.h"
#include "module/rnd.h"

void
uniqport(p)
port *	p;
{
    register char *	c;
    register short	r;
    register int	n;
    static int		been_here;

    if (!been_here)
    {
	long	time();
	long	t;

	/*
	 * Sigh.. Heavy danger zone ahead
	 *
	 * Originally this code added the PID and upper and lower
	 * parts of the time to one 16 bit seed and used that.
	 * Until the moment when two servers started at about the
	 * same time did their uniqports in reversed order and
	 * generated the same ports.
	 *
	 * For our next trick we took away the time code, in the
	 * mistaken belief that that could hardly be worse.
	 * Then came the time that the SOAP server's port,
	 * originally generated under UNIX was regenerated by a
	 * process under UNIX that happened to have the same PID
	 * as the one that generated the SOAP port.
	 *
	 * Now we are almost back to the first, only we make the
	 * seed a bit bigger (32 bits) and we shift the PID before
	 * adding it to the time.
	 * The shift should be big enough to prevent the first
	 * anomaly from occurring, but not so big there is a chance
	 * of PID wraparound. After long careful study we chose 8 :-)
	 *
	 * The real solution would be hardware. We don't have it.
	 */
	time(&t);
	srand((int) (t + (getpid() << 8)));
	been_here = 1;
    }
    do
    {
	c = (char *) p;
	n = PORTSIZE / 2;
	do
	{
	    r = rand();
	    *c++ ^= r >> 8;
	    *c++ ^= r & 0377;
	} while (--n);
    } while (NULLPORT(p));
}