/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for ZReceiveNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZRecvNot.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZRecvNot.c,v 1.7 1988-06-15 16:55:51 rfrench Exp $ */

#ifndef lint
static char rcsid_ZReceiveNotice_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZRecvNot.c,v 1.7 1988-06-15 16:55:51 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZReceiveNotice(notice, from)
    ZNotice_t *notice;
    struct sockaddr_in *from;
{
    char *buffer;
    struct _Z_InputQ *nextq;
    int len;
    Code_t retval;

    if ((retval = Z_WaitForComplete()) != ZERR_NONE)
	return (retval);

    nextq = (struct _Z_InputQ *) Z_GetFirstComplete();

    len = nextq->packet_len;
    
    if (!(buffer = malloc(len)))
	return (ENOMEM);

    if (from)
	*from = nextq->from;
    
    bcopy(nextq->packet, buffer, len);

    (void) Z_RemQueue(nextq);
    
    return (ZParseNotice(buffer, len, notice));
}
