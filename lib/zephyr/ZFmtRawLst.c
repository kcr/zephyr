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
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtRawLst.c,v 1.4 1988-05-17 21:21:49 rfrench Exp $ */

#ifndef lint
static char rcsid_ZFormatRawNoticeList_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtRawLst.c,v 1.4 1988-05-17 21:21:49 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZFormatRawNoticeList(notice, list, nitems, buffer, ret_len)
    ZNotice_t *notice;
    char *list[];
    int nitems;
    char **buffer;
    int *ret_len;
{
    char header[Z_MAXHEADERLEN];
    int hdrlen, i, size;
    char *ptr;
    Code_t retval;

    if ((retval = Z_FormatRawHeader(notice, header, sizeof(header), &hdrlen))
	!= ZERR_NONE)
	return (retval);

    size = 0;
    for (i=0;i<nitems;i++)
	size += strlen(list[i])+1;

    *ret_len = hdrlen+size;
    
    if (!(*buffer = malloc(*ret_len)))
	return (ENOMEM);

    bcopy(header, *buffer, hdrlen);
    
    ptr = *buffer+hdrlen;

    for (;nitems;nitems--, list++) {
	i = strlen(*list)+1;
	bcopy(*list, ptr, i);
	ptr += i;
    }

    return (ZERR_NONE);
}
