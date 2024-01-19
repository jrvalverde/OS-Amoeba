/*	@(#)tune.c	1.2	96/02/27 13:00:42 */
/* Changes should be made in Makefile, not to this file! */

/***************************************************************************
 * This program is Copyright (C) 1986, 1987, 1988 by Jonathan Payne.  JOVE *
 * is provided to you without charge, and with no warranty.  You may give  *
 * away copies of JOVE, including sources, provided that this notice is    *
 * included in all the files.                                              *
 ***************************************************************************/

#include "jove.h"

char	*d_tempfile = "joveXXXXXX",	/* buffer lines go here */
	*p_tempfile = "jrecXXXXXX",	/* line pointers go here */
	*Recover = "/profile/module/jove/recover",
	*CmdDb = "/profile/module/jove/cmds.doc",
		/* copy of "cmds.doc" lives in the doc subdirectory */

	*Joverc = "/profile/module/jove/jove.rc",

#if defined(IPROCS) && defined(PIPEPROCS)
	*Portsrv = "/profile/module/jove/portsrv",
	*Kbd_Proc = "/profile/module/jove/kbd",
#endif

/* these are variables that can be set with the set command, so they are
   allocated more memory than they actually need for the defaults */

	TmpFilePath[FILESIZE] = "/usr/tmp",
	Shell[FILESIZE] = "/bin/sh",
	ShFlags[16] = "-c";
