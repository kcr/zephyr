/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFormatNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtNotice.c,v $
 *	$Author: jfc $
 *
 *	Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtNotice.c,v 1.12 1991-06-20 14:24:54 jfc Exp $ */

#ifndef lint
static char rcsid_ZFormatNotice_c[] = "$Id: ZFmtNotice.c,v 1.12 1991-06-20 14:24:54 jfc Exp $";
#endif

#include <zephyr/zephyr_internal.h>

Code_t ZFormatNotice(notice, buffer, ret_len, cert_routine)
    register ZNotice_t *notice;
    char **buffer;
    int *ret_len;
    Z_AuthProc cert_routine;
{
    char header[Z_MAXHEADERLEN];
    int hdrlen;
    Code_t retval;

    if ((retval = Z_FormatHeader(notice, header, sizeof(header), &hdrlen, 
				 cert_routine)) != ZERR_NONE)
	return (retval);

    *ret_len = hdrlen+notice->z_message_len;

    /* Length can never be zero, don't have to worry about malloc(0). */
    if (!(*buffer = malloc((unsigned)*ret_len)))
	return (ENOMEM);

    bcopy(header, *buffer, hdrlen);
    bcopy(notice->z_message, *buffer+hdrlen, notice->z_message_len);

    return (ZERR_NONE);
}