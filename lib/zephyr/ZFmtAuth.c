/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFormatAuthenticNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtAuth.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtAuth.c,v 1.9 1988-11-14 11:29:03 jtkohl Exp $ */

#ifndef lint
static char rcsid_ZFormatAuthenticNotice_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFmtAuth.c,v 1.9 1988-11-14 11:29:03 jtkohl Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

#ifdef KERBEROS
Code_t ZFormatAuthenticNotice(notice, buffer, buffer_len, len, session)
    ZNotice_t *notice;
    char *buffer;
    int buffer_len;
    int *len;
    C_Block session;
{
    ZNotice_t newnotice;
    char *ptr;
    int retval, hdrlen;

    newnotice = *notice;
    newnotice.z_auth = 1;
    newnotice.z_authent_len = 0;
    newnotice.z_ascii_authent = "";
	
    if ((retval = Z_FormatRawHeader(&newnotice, buffer, buffer_len,
				    &hdrlen, &ptr)) != ZERR_NONE)
	return (retval);

    newnotice.z_checksum = (ZChecksum_t)quad_cksum(buffer, NULL, ptr - buffer,
						   0, session);

    if ((retval = Z_FormatRawHeader(&newnotice, buffer, buffer_len,
				    &hdrlen, (char **) 0)) != ZERR_NONE)
	return (retval);

    ptr = buffer+hdrlen;

    if (newnotice.z_message_len+hdrlen > buffer_len)
	return (ZERR_PKTLEN);

    bcopy(newnotice.z_message, ptr, newnotice.z_message_len);

    *len = hdrlen+newnotice.z_message_len;

    if (*len > Z_MAXPKTLEN)
	return (ZERR_PKTLEN);

    return (ZERR_NONE);
}
#endif
