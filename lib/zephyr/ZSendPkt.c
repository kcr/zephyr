/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSendPacket function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZSendPkt.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZSendPkt.c,v 1.20 1988-05-13 15:17:53 rfrench Exp $ */

#ifndef lint
static char rcsid_ZSendPacket_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZSendPkt.c,v 1.20 1988-05-13 15:17:53 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>
#include <sys/socket.h>

Code_t ZSendPacket(packet, len, waitforack)
    char *packet;
    int len;
    int waitforack;
{
    Code_t retval;
    struct sockaddr_in dest;
    struct timeval tv;
    int i;
    fd_set t1, t2, t3;
    char *ackpacket;
    ZNotice_t notice, acknotice;
	
    if (!packet || len < 0 || len > Z_MAXPKTLEN)
	return (ZERR_ILLVAL);

    if (ZGetFD() < 0)
	if ((retval = ZOpenPort((u_short *)0)) != ZERR_NONE)
	    return (retval);

    dest = ZGetDestAddr();
	
    if (sendto(ZGetFD(), packet, len, 0, &dest, sizeof(dest)) < 0)
	return (errno);

    if (!waitforack)
	return (ZERR_NONE);

    if ((retval = ZParseNotice(packet, len, &notice)) != ZERR_NONE)
	return (retval);
    
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    /* XXX */
    for (i=0;i<HM_TIMEOUT*2;i++) {
	if (select(0, &t1, &t2, &t3, &tv) < 0)
	    return (errno);
	retval = ZCheckIfNotice(ackpack, sizeof(ackpack), &acknotice, 
				NULL, ZCompareUIDPred, 
				(char *)&notice.z_uid);
	if (retval == ZERR_NONE)
	    return (ZERR_NONE);
	if (retval != ZERR_NONOTICE)
	    return (retval);
    }
    return (ZERR_HMDEAD);
}
