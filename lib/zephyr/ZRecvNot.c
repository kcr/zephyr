/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for ZReceiveNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZRecvNot.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZRecvNot.c,v 1.9 1988-06-29 16:40:40 jtkohl Exp $ */

#ifndef lint
static char rcsid_ZReceiveNotice_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZRecvNot.c,v 1.9 1988-06-29 16:40:40 jtkohl Exp $";
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

    nextq = Z_GetFirstComplete();

    len = nextq->packet_len;
    
    if (!(buffer = malloc((unsigned) len)))
	return (ENOMEM);

    if (from)
	*from = nextq->from;
    
    bcopy(nextq->packet, buffer, len);

    Z_RemQueue(nextq);
    
    return (ZParseNotice(buffer, len, notice));
}
