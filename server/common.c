/* This file is part of the Project Athena Zephyr Notification System.
 * It contains functions for general use within the Zephyr server.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /srv/kcr/athena/zephyr/server/common.c,v $
 *	$Author: raeburn $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#ifndef lint
#ifndef SABER
static const char rcsid_common_c[] =
    "$Header: /srv/kcr/athena/zephyr/server/common.c,v 1.8 1991-01-28 15:06:40 raeburn Exp $";
#endif SABER
#endif lint

#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include "zserver.h"

/* common routines for the server */

/* copy the string into newly allocated area */

char *
strsave (const char *sp)
{
    register char *ret;

    if((ret = (char *) xmalloc((unsigned) strlen(sp)+1)) == NULL) {
	    syslog(LOG_ERR, "no mem strdup'ing");
	    abort();
    }
    (void) strcpy(ret,sp);
    return(ret);
}

/* generic string hash function */

unsigned long
hash (const char *string)
{
	register unsigned long hval = 0;
	register char cp;

	while (1) {
	    cp = *string++;
	    if (!cp)
		break;
	    hval += cp;

	    cp = *string++;
	    if (!cp)
		break;
	    hval += cp * (3 + (1 << 16));

	    cp = *string++;
	    if (!cp)
		break;
	    hval += cp * (1 + (1 << 8));

	    cp = *string++;
	    if (!cp)
		break;
	    hval += cp * (1 + (1 << 12));

	    cp = *string++;
	    if (!cp)
		break;
	    hval += cp * (1 + (1 << 4));

	    hval += ((long) hval) >> 18;
	}
	hval &= 0x7fffffff;
	return hval;
}
