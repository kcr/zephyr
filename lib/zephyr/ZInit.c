/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZInitialize function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZInit.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZInit.c,v 1.10 1988-05-13 14:17:37 rfrench Exp $ */

#ifndef lint
static char rcsid_ZInitialize_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZInit.c,v 1.10 1988-05-13 14:17:37 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/param.h>

Code_t ZInitialize()
{
    struct servent *hmserv;
    char addr[4];
    int retval;
#ifndef KERBEROS
    char hostname[MAXHOSTNAMELEN+1];
    struct hostent *hent;
#endif
    
    init_zeph_err_tbl();
#ifdef KERBEROS
    init_krb_err_tbl();
#endif
    
    bzero((char *)&__HM_addr, sizeof(__HM_addr));

    __HM_addr.sin_family = AF_INET;

    /* Set up local loopback address for HostManager */
    addr[0] = 127;
    addr[1] = 0;
    addr[2] = 0;
    addr[3] = 1;

    hmserv = (struct servent *)getservbyname("zephyr-hm", "udp");
    if (!hmserv)
	return (ZERR_HMPORT);

    __HM_addr.sin_port = hmserv->s_port;

    bcopy(addr, (char *)&__HM_addr.sin_addr, 4);

    __HM_set = 0;

#ifdef KERBEROS    
    if ((retval = get_krbrlm(__Zephyr_realm, 1)) != KSUCCESS)
	return (retval);
#else
    if (gethostname(hostname, MAXHOSTNAMELEN))
	return (errno);
    if (hent = gethostbyname(hostname))
	strcpy(__Zephyr_realm, hent->h_name);
    else
	strcpy(__Zephyr_realm, hostname);
#endif

    /* Get the sender so we can cache it */
    (void) ZGetSender();
	
    return (ZERR_NONE);
}
