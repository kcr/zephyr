/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFormatSmallRawNoticeList function.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtSmRLst.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtSmRLst.c,v 1.8 1995-06-30 22:04:06 ghudson Exp $ */

#ifndef lint
static char rcsid_ZFormatRawNoticeList_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtSmRLst.c,v 1.8 1995-06-30 22:04:06 ghudson Exp $";
#endif

#include <internal.h>

Code_t ZFormatSmallRawNoticeList(notice, list, nitems, buffer, ret_len)
    ZNotice_t *notice;
    char *list[];
    int nitems;
    ZPacket_t buffer;
    int *ret_len;
{
    Code_t retval;
    int hdrlen, i, size;
    char *ptr;

    if ((retval = Z_FormatRawHeader(notice, buffer, Z_MAXHEADERLEN,
				    &hdrlen, NULL, NULL)) != ZERR_NONE)
	return (retval);

    size = 0;
    for (i=0;i<nitems;i++)
	size += strlen(list[i])+1;

    *ret_len = hdrlen+size;

    if (*ret_len > Z_MAXPKTLEN)
	return (ZERR_PKTLEN);

    ptr = buffer+hdrlen;

    for (;nitems;nitems--, list++) {
	i = strlen(*list)+1;
	(void) memcpy(ptr, *list, i);
	ptr += i;
    }

    return (ZERR_NONE);
}
