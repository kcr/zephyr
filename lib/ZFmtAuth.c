/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFormatAuthenticNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZFmtAuth.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZFmtAuth.c,v 1.3 1987-06-29 17:55:43 rfrench Exp $ */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZFormatAuthenticNotice(notice,buffer,buffer_len,len,session)
	ZNotice_t	*notice;
	ZPacket_t	buffer;
	int		buffer_len;
	int		*len;
	C_Block		session;
{
	char *ptr;
	int retval,hdrlen;

	notice->z_auth = 1;
	notice->z_authent_len = 0;
	notice->z_ascii_authent = (char *)"";
	
	if ((retval = Z_FormatRawHeader(notice,buffer,buffer_len,&hdrlen))
	    != ZERR_NONE)
		return (retval);

	for (hdrlen--;buffer[hdrlen-1];hdrlen--)
		;
	
/*	if (result = get_credentials(SERVER_SERVICE,SERVER_INSTANCE,
			    __Zephyr_realm,&cred))
		return (result+krb_err_base);
*/
	notice->z_checksum = (ZChecksum_t)quad_cksum(buffer,NULL,hdrlen,0,
						     session);

	if ((retval = Z_FormatRawHeader(notice,buffer,buffer_len,&hdrlen))
	    != ZERR_NONE)
		return (retval);

	ptr = buffer+hdrlen;

	if (notice->z_message_len+hdrlen > buffer_len)
		return (ZERR_PKTLEN);

	bcopy(notice->z_message,ptr,notice->z_message_len);

	*len = hdrlen+notice->z_message_len;

	if (*len > Z_MAXPKTLEN)
		return (ZERR_PKTLEN);

	return (ZERR_NONE);
}
