/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSendPacket function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZSendPkt.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZSendPkt.c,v 1.22 1988-05-17 21:23:46 rfrench Exp $ */

#ifndef lint
static char rcsid_ZSendPacket_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZSendPkt.c,v 1.22 1988-05-17 21:23:46 rfrench Exp $";
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
    ZNotice_t notice, acknotice;
	
    if (!packet || len < 0)
	return (ZERR_ILLVAL);

    if (len > Z_MAXPKTLEN)
	return (ZERR_PKTLEN);
    
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

    for (i=0;i<HM_TIMEOUT*2;i++) {
	if (select(0, &t1, &t2, &t3, &tv) < 0)
	    return (errno);
	retval = ZCheckIfNotice(&acknotice, (struct sockaddr_in *)0,
				ZCompareUIDPred, (char *)&notice.z_uid);
	if (retval == ZERR_NONE) {
	    ZFreeNotice(&acknotice);
	    return (ZERR_NONE);
	}
	if (retval != ZERR_NONOTICE)
	    return (retval);
    }
    return (ZERR_HMDEAD);
}
