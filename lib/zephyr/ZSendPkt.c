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
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZSendPkt.c,v 1.1 1987-06-10 12:35:43 rfrench Exp $ */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>
#include <sys/socket.h>

Code_t ZSendPacket(packet,len)
	ZPacket_t	packet;
	int		len;
{
	Code_t retval;
	struct sockaddr_in sin;

	if (!packet || len < 0 || len > Z_MAXPKTLEN)
		return (ZERR_ILLVAL);

	if (ZGetFD() < 0)
		if ((retval = ZOpenPort(0)) != ZERR_NONE)
			return (retval);

	if ((retval = Z_GetHMPortAddr()) != ZERR_NONE)
		return (retval);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(__HM_port);
	bcopy(__HM_addr,&sin.sin_addr,__HM_length);

	printf("Internal: Sending to port %d\n",__HM_port);

	if (sendto(ZGetFD(),packet,len,0,&sin,sizeof(sin)) < 0)
		return (ZERR_UNIX);

	return (ZERR_NONE);
}
