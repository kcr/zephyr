/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZMakeAuthentication function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZMkAuth.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZMkAuth.c,v 1.4 1988-06-17 17:15:57 jtkohl Exp $ */

#ifndef lint
static char rcsid_ZMakeAuthentication_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZMkAuth.c,v 1.4 1988-06-17 17:15:57 jtkohl Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZMakeAuthentication(notice, buffer, buffer_len, len)
    ZNotice_t *notice;
    char *buffer;
    int buffer_len;
    int *len;
{
#ifdef KERBEROS
    int retval, result;
    KTEXT_ST authent;

    notice->z_auth = 1;
    if ((result = krb_mk_req(&authent, SERVER_SERVICE, 
			     SERVER_INSTANCE, __Zephyr_realm, 0))
	!= MK_AP_OK)
	return (result+krb_err_base);
    notice->z_authent_len = authent.length;
    notice->z_ascii_authent = (char *)malloc((unsigned)authent.length*3);
    if (!notice->z_ascii_authent)
	return (ENOMEM);
    if ((retval = ZMakeAscii(notice->z_ascii_authent, 
			     authent.length*3, 
			     authent.dat, 
			     authent.length)) != ZERR_NONE) {
	free(notice->z_ascii_authent);
	return (retval);
    }
    retval = Z_FormatRawHeader(notice, buffer, buffer_len, len, (char **) 0);
    free(notice->z_ascii_authent);
    notice->z_authent_len = 0;

    return (retval);
#else
    notice->z_authent_len = 0;
    return (Z_FormatRawHeader(notice, buffer, buffer_len, len, (char **) 0));
#endif
}
