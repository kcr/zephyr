/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFormatSmallRawNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtSmRaw.c,v $
 *	$Author: probe $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtSmRaw.c,v 1.4 1993-09-24 16:19:16 probe Exp $ */

#ifndef lint
static char rcsid_ZFormatRawNotice_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtSmRaw.c,v 1.4 1993-09-24 16:19:16 probe Exp $";
#endif

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZFormatSmallRawNotice(notice, buffer, ret_len)
    ZNotice_t *notice;
    ZPacket_t buffer;
    int *ret_len;
{
    Code_t retval;
    int hdrlen;
    
    if ((retval = Z_FormatRawHeader(notice, buffer, Z_MAXHEADERLEN,
				    &hdrlen, (char **) 0)) != ZERR_NONE)
	return (retval);

    *ret_len = hdrlen+notice->z_message_len;

    if (*ret_len > Z_MAXPKTLEN)
	return (ZERR_PKTLEN);

    _BCOPY(notice->z_message, buffer+hdrlen, notice->z_message_len);

    return (ZERR_NONE);
}
