/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSendNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZSendNot.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZSendNot.c,v 1.7 1987-07-29 15:18:22 rfrench Exp $ */

#ifndef lint
static char rcsid_ZSendNotice_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZSendNot.c,v 1.7 1987-07-29 15:18:22 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZSendNotice(notice,cert_routine)
	ZNotice_t	*notice;
	int		(*cert_routine)();
{
	Code_t retval;
	char *buffer;
	int len;

	buffer = (char *)malloc(Z_MAXPKTLEN);
	if (!buffer)
		return (ENOMEM);

	if ((retval = ZFormatNotice(notice,buffer,Z_MAXPKTLEN,&len,
				    cert_routine)) != ZERR_NONE) {
		free(buffer);
		return (retval);
	}

	retval = ZSendPacket(buffer,len);
	free(buffer);

	return (retval);
}
