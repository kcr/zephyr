/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for ZReceivePacket function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZRecvPkt.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZRecvPkt.c,v 1.8 1987-07-29 15:18:05 rfrench Exp $ */

#ifndef lint
static char rcsid_ZReceivePacket_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZRecvPkt.c,v 1.8 1987-07-29 15:18:05 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

#define min(a,b) ((a)<(b)?(a):(b))
	
Code_t ZReceivePacket(buffer,buffer_len,ret_len,from)
	ZPacket_t	buffer;
	int		buffer_len;
	int		*ret_len;
	struct		sockaddr_in *from;
{
	int retval;
	
	if (ZGetFD() < 0)
		return (ZERR_NOPORT);

	if (!ZQLength())
		if ((retval = Z_ReadWait()) != ZERR_NONE)
			return (retval);

	if (buffer_len < __Q_Head->packet_len) {
		*ret_len = buffer_len;
		retval = ZERR_PKTLEN;
	}
	else {
		*ret_len = __Q_Head->packet_len;
		retval = ZERR_NONE;
	}

	if (ret_len)
		bcopy(__Q_Head->packet,buffer,*ret_len);
	if (from)
		bcopy((char *)&__Q_Head->from,(char *)from,
		      sizeof(struct sockaddr_in));
	
	(void) Z_RemQueue(__Q_Head);

	return (retval);
}
