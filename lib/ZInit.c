/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZInitialize function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZInit.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZInit.c,v 1.6 1987-06-24 04:20:21 rfrench Exp $ */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>
#include <netdb.h>
#include <sys/socket.h>

Code_t ZInitialize()
{
	struct servent *hmserv;
	char addr[4];
	
	init_zeph_err_tbl();
	init_krb_err_tbl();
	
	bzero(&__HM_addr,sizeof(__HM_addr));

	__HM_addr.sin_family = AF_INET;
	
	
	addr[0] = 127;
	addr[1] = 0;
	addr[2] = 0;
	addr[3] = 1;
	
	hmserv = (struct servent *)getservbyname("zephyr-hm","udp");
	if (!hmserv)
		return (ZERR_HMPORT);

	__HM_addr.sin_port = hmserv->s_port;

	bcopy(addr,&__HM_addr.sin_addr,4);

	__HM_set = 0;
	
	if (get_krbrlm(__Zephyr_realm,1) != KSUCCESS)
		strcpy(__Zephyr_realm,"NOREALM");

	return (ZERR_NONE);
}
