/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFormatRawNoticeList function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtRawLst.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtRawLst.c,v 1.2 1987-07-29 15:16:11 rfrench Exp $ */

#ifndef lint
static char rcsid_ZFormatRawNoticeList_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtRawLst.c,v 1.2 1987-07-29 15:16:11 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr.h>

Code_t ZFormatRawNoticeList(notice,list,nitems,buffer,buffer_len,ret_len)
	ZNotice_t	*notice;
	char		*list[];
	int		nitems;
	ZPacket_t	buffer;
	int		buffer_len;
	int		*ret_len;
{
	char *ptr,*end;
	Code_t retval;

	end = buffer+buffer_len;

	if ((retval = Z_FormatRawHeader(notice,buffer,buffer_len,ret_len)) !=
	    ZERR_NONE)
		return (retval);

	ptr = buffer+*ret_len;

	for (;nitems;nitems--,list++) {
		if (ptr+strlen(*list)+1 > end)
			return (ZERR_PKTLEN);
		bcopy(*list,ptr,strlen(*list)+1);
		*ret_len += strlen(*list)+1;
		ptr += strlen(*list)+1;
	}

	if (*ret_len > Z_MAXPKTLEN)
		return (ZERR_PKTLEN);

	return (ZERR_NONE);
}
