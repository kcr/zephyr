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
/* $Header: /srv/kcr/athena/zephyr/lib/ZSendPkt.c,v 1.11 1987-06-24 04:21:57 rfrench Exp $ */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>
#include <sys/socket.h>

Code_t ZSendPacket(packet,len)
	ZPacket_t	packet;
	int		len;
{
	int findack();
	
	Code_t retval;
	struct sockaddr_in dest;
	struct timeval tv;
	int auth,t1,t2,t3,i;
	ZPacket_t ackpack;
	ZNotice_t notice;
	
	if (!packet || len < 0 || len > Z_MAXPKTLEN)
		return (ZERR_ILLVAL);

	if (ZGetFD() < 0)
		if ((retval = ZOpenPort(0)) != ZERR_NONE)
			return (retval);

	dest = ZGetDestAddr();
	
	if (sendto(ZGetFD(),packet,len,0,&dest,sizeof(dest)) < 0)
		return (errno);

	ZParseNotice(packet,len,&notice,0,0);

	if (notice.z_kind == UNSAFE || notice.z_kind == HMACK ||
	    notice.z_kind == SERVACK || __HM_set)
		return (ZERR_NONE);
	
	tv.tv_sec = 0;
	tv.tv_usec = 400000;
	
	for (i=0;i<12;i++) {
		select(0,&t1,&t2,&t3,&tv);
		retval = ZCheckIfNotice(ackpack,sizeof ackpack,&notice,
					&auth,findack,(char *)&notice.z_uid);
		if (retval == ZERR_NONE)
			return (ZERR_NONE);
		if (retval != ZERR_NONOTICE)
			return (retval);
	}
	return (ZERR_HMDEAD);
}

static int findack(notice,uid)
	ZNotice_t *notice;
	ZUnique_Id_t *uid;
{
	return (ZCompareUID(uid,&notice->z_uid));
}
