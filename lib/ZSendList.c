/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSendList function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZSendList.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZSendList.c,v 1.6 1987-07-29 15:18:17 rfrench Exp $ */

#ifndef lint
static char rcsid_ZSendList_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZSendList.c,v 1.6 1987-07-29 15:18:17 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZSendList(notice,list,nitems,cert_routine)
	ZNotice_t	*notice;
	char		*list[];
	int		nitems;
	int		(*cert_routine)();
{
	Code_t retval;
	char *buffer;
	int len;

	buffer = (char *)malloc(Z_MAXPKTLEN);
	if (!buffer)
		return (ENOMEM);

	if ((retval = ZFormatNoticeList(notice,list,nitems,buffer,
					Z_MAXPKTLEN,&len,cert_routine))
	    != ZERR_NONE) {
		free(buffer);
		return (retval);
	}

	retval = ZSendPacket(buffer,len);
	free(buffer);

	return (retval);
}
