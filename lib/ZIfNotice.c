/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZIfNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZIfNotice.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZIfNotice.c,v 1.6 1988-05-17 21:22:16 rfrench Exp $ */

#ifndef lint
static char rcsid_ZIfNotice_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZIfNotice.c,v 1.6 1988-05-17 21:22:16 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZIfNotice(notice, from, predicate, args)
    ZNotice_t *notice;
    struct sockaddr_in *from;
    int (*predicate)();
    char *args;
{
    ZNotice_t tmpnotice;
    Code_t retval;
    char *buffer;
    struct _Z_InputQ *qptr;

    if (ZQLength())
	retval = Z_ReadEnqueue();
    else
	retval = Z_ReadWait();
	
    if (retval != ZERR_NONE)
	return (retval);
	
    qptr = (struct _Z_InputQ *) Z_GetFirstComplete();
    
    for (;;) {
	while (qptr) {
	    if ((retval = ZParseNotice(qptr->packet, qptr->packet_len, 
				       &tmpnotice)) != ZERR_NONE)
		return (retval);
	    if ((predicate)(&tmpnotice, args)) {
		if (!(buffer = malloc(qptr->packet_len)))
		    return (ENOMEM);
		bcopy(qptr->packet, buffer, qptr->packet_len);
		if (from)
		    *from = qptr->from;
		if ((retval = ZParseNotice(buffer, qptr->packet_len, 
					   notice)) != ZERR_NONE) {
		    free(buffer);
		    return (retval);
		}
		return (Z_RemQueue(qptr));
	    }
	    qptr = Z_GetNextComplete(qptr);
	}
	if ((retval = Z_ReadWait()) != ZERR_NONE)
	    return (retval);
    }
}
