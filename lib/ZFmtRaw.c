/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFormatRawNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZFmtRaw.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZFmtRaw.c,v 1.4 1988-05-17 21:21:44 rfrench Exp $ */

#ifndef lint
static char rcsid_ZFormatRawNotice_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZFmtRaw.c,v 1.4 1988-05-17 21:21:44 rfrench Exp $";
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

    if ((retval = Z_FormatRawHeader(notice, header, sizeof(header), &hdrlen))
	!= ZERR_NONE)
	return (retval);

    *ret_len = hdrlen+notice->z_message_len;

    if (!(*buffer = malloc(*ret_len)))
	return (ENOMEM);

    bcopy(header, *buffer, hdrlen);
    bcopy(notice->z_message, *buffer+hdrlen, notice->z_message_len);

    return (ZERR_NONE);
}
