/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for ZReceivePacket function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZRecvPkt.c,v $
 *	$Author: probe $
 *
 *	Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZRecvPkt.c,v 1.15 1993-11-19 15:25:48 probe Exp $ */

#ifndef lint
static char rcsid_ZReceivePacket_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZRecvPkt.c,v 1.15 1993-11-19 15:25:48 probe Exp $";
#endif

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZReceivePacket(buffer, ret_len, from)
    ZPacket_t buffer;
    int *ret_len;
    struct sockaddr_in *from;
{
    Code_t retval;
    struct _Z_InputQ *nextq;
    
    if ((retval = Z_WaitForComplete()) != ZERR_NONE)
	return (retval);

    nextq = Z_GetFirstComplete();

    *ret_len = nextq->packet_len;
    if (*ret_len > Z_MAXPKTLEN)
	return (ZERR_PKTLEN);
    
    (void) memcpy(buffer, nextq->packet, *ret_len);

    if (from)
	*from = nextq->from;
	
    Z_RemQueue(nextq);

    return (ZERR_NONE);
}
