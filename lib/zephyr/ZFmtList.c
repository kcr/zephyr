/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFormatNoticeList function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtList.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtList.c,v 1.6 1988-05-13 18:11:01 rfrench Exp $ */

#ifndef lint
static char rcsid_ZFormatNoticeList_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtList.c,v 1.6 1988-05-13 18:11:01 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZFormatNoticeList(notice, list, nitems, buffer, ret_len, 
			 cert_routine)
    ZNotice_t *notice;
    char *list[];
    int nitems;
    char **buffer;
    int *ret_len;
    int (*cert_routine)();
{
    char header[Z_MAXHEADERLEN];
    int hdrlen, i, size;
    char *ptr, *end;
    Code_t retval;

    if ((retval = Z_FormatHeader(notice, header, sizeof(header), &hdrlen,
				 cert_routine)) != ZERR_NONE)
	return (retval);

    size = 0;
    for (i=0;i<nitems;i++)
	size += strlen(list[i])+1;

    *ret_len = hdrlen+size;
    
    if (!(*buffer = malloc(*ret_len)))
	return (ENOMEM);

    ptr = *buffer+hdrlen;

    for (;nitems;nitems--, list++) {
	i = strlen(*list)+1;
	bcopy(*list, ptr, i);
	ptr += i;
    }

    return (ZERR_NONE);
}
