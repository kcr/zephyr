/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for ZPeekPacket function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZPeekPkt.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZPeekPkt.c,v 1.12 1995-06-30 22:04:31 ghudson Exp $ */

#ifndef lint
static char rcsid_ZPeekPacket_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZPeekPkt.c,v 1.12 1995-06-30 22:04:31 ghudson Exp $";
#endif

#include <internal.h>

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
