/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for ZReceiveNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZRecvNot.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZRecvNot.c,v 1.2 1987-06-20 19:21:43 rfrench Exp $ */

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

	return (ZParseNotice(buffer,len,notice,auth));
}
