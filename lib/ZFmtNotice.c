/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFormatNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZFmtNotice.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZFmtNotice.c,v 1.6 1988-05-13 18:04:40 rfrench Exp $ */

#ifndef lint
static char rcsid_ZFormatNotice_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZFmtNotice.c,v 1.6 1988-05-13 18:04:40 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr.h>

Code_t ZFormatNotice(notice, buffer, len, cert_routine)
    ZNotice_t *notice;
    char **buffer;
    int *len;
    int (*cert_routine)();
{
    char header[Z_MAXHEADERLEN];
    char *ptr;
    int hdrlen;
    Code_t retval;

    if ((retval = Z_FormatHeader(notice, header, sizeof(header), &hdrlen, 
				 cert_routine)) != ZERR_NONE)
	return (retval);

    *len = hdrlen+notice->z_message_len;

    if (!(*buffer = malloc(*len)))
	return (ENOMEM);

    notice->z_packet = *buffer;
    
    bcopy(notice->z_message, *buffer+hdrlen, notice->z_message_len);

    return (ZERR_NONE);
}
