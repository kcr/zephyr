/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFormatRawNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtRaw.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtRaw.c,v 1.6 1988-06-23 10:29:22 jtkohl Exp $ */

#ifndef lint
static char rcsid_ZFormatRawNotice_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtRaw.c,v 1.6 1988-06-23 10:29:22 jtkohl Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZFormatRawNotice(notice, buffer, ret_len)
    ZNotice_t *notice;
    char **buffer;
    int *ret_len;
{
    char header[Z_MAXHEADERLEN];
    int hdrlen;
    Code_t retval;

    if ((retval = Z_FormatRawHeader(notice, header, sizeof(header),
				    &hdrlen, (char **) 0)) != ZERR_NONE)
	return (retval);

    *ret_len = hdrlen+notice->z_message_len;

    if (!(*buffer = malloc((unsigned) *ret_len)))
	return (ENOMEM);

    bcopy(header, *buffer, hdrlen);
    bcopy(notice->z_message, *buffer+hdrlen, notice->z_message_len);

    return (ZERR_NONE);
}
