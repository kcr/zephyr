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
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZRecvNot.c,v 1.4 1987-07-29 15:17:59 rfrench Exp $ */

#ifndef lint
static char rcsid_ZReceiveNotice_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZRecvNot.c,v 1.4 1987-07-29 15:17:59 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZReceiveNotice(buffer,buffer_len,notice,auth,from)
	ZPacket_t	buffer;
	int		buffer_len;
	ZNotice_t	*notice;
	int		*auth;
	struct		sockaddr_in *from;
{
	int len;
	Code_t retval;
	
	if ((retval = ZReceivePacket(buffer,buffer_len,&len,from)) !=
	    ZERR_NONE)
		return (retval);

	return (ZParseNotice(buffer,len,notice,auth,from));
}
