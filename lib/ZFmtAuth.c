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
/* $Header: /srv/kcr/athena/zephyr/lib/ZFmtAuth.c,v 1.4 1987-07-09 20:02:18 rfrench Exp $ */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZFormatAuthenticNotice(notice,buffer,buffer_len,len,session)
	ZNotice_t	*notice;
	ZPacket_t	buffer;
	int		buffer_len;
	int		*len;
	C_Block		session;
{
	ZNotice_t newnotice;
	char *ptr;
	int retval,hdrlen;

	newnotice = *notice;
	newnotice.z_auth = 1;
	newnotice.z_authent_len = 0;
	newnotice.z_ascii_authent = (char *)"";
	
	if ((retval = Z_FormatRawHeader(&newnotice,buffer,buffer_len,&hdrlen))
	    != ZERR_NONE)
		return (retval);

	for (hdrlen--;buffer[hdrlen-1];hdrlen--)
		;
	
	newnotice.z_checksum = (ZChecksum_t)quad_cksum(buffer,NULL,hdrlen,0,
						     session);

	if ((retval = Z_FormatRawHeader(&newnotice,buffer,buffer_len,&hdrlen))
	    != ZERR_NONE)
		return (retval);

	ptr = buffer+hdrlen;

	if (newnotice.z_message_len+hdrlen > buffer_len)
		return (ZERR_PKTLEN);

	bcopy(newnotice.z_message,ptr,newnotice.z_message_len);

	*len = hdrlen+newnotice.z_message_len;

	if (*len > Z_MAXPKTLEN)
		return (ZERR_PKTLEN);

	return (ZERR_NONE);
}
