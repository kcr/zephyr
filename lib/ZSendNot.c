/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSendNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZSendNot.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZSendNot.c,v 1.3 1987-06-12 18:46:33 rfrench Exp $ */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZSendNotice(notice)
	ZNotice_t	*notice;
{
	Code_t retval;
	char *buffer;
	int len;

	buffer = (char *)malloc(Z_MAXPKTLEN);
	if (!buffer)
		return (ZERR_NOMEM);

	if ((retval = ZFormatNotice(notice,buffer,Z_MAXPKTLEN,&len)) !=
	    ZERR_NONE) {
		free(buffer);
		return (retval);
	}

	retval = ZSendPacket(buffer,len);
	free(buffer);

	return (retval);
}
