/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for ZPeekPacket function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZPeekPkt.c,v $
 *	$Author: probe $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZPeekPkt.c,v 1.11 1993-11-19 15:25:45 probe Exp $ */

#ifndef lint
static char rcsid_ZPeekPacket_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZPeekPkt.c,v 1.11 1993-11-19 15:25:45 probe Exp $";
#endif

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZPeekPacket(buffer, ret_len, from)
    char **buffer;
    int *ret_len;
    struct sockaddr_in *from;
{
    Code_t retval;
    struct _Z_InputQ *nextq;
    
    if ((retval = Z_WaitForComplete()) != ZERR_NONE)
	return (retval);

    nextq =Z_GetFirstComplete();

    *ret_len = nextq->packet_len;
    
    if (!(*buffer = (char *) malloc((unsigned) *ret_len)))
	return (ENOMEM);

    (void) memcpy(*buffer, nextq->packet, *ret_len);

    if (from)
	*from = nextq->from;
	
    return (ZERR_NONE);
}
