/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZInitialize function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZInit.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZInit.c,v 1.17 1989-05-30 18:11:25 jtkohl Exp $ */

#ifndef lint
static char rcsid_ZInitialize_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZInit.c,v 1.17 1989-05-30 18:11:25 jtkohl Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/param.h>
#ifdef KERBEROS
#include "krb_err.h"
#endif

Code_t ZInitialize()
{
    struct servent *hmserv;
    char addr[4];
#ifdef KERBEROS
    int krbval;
#endif
    
    initialize_zeph_error_table();
#ifdef KERBEROS
    initialize_krb_error_table();
#endif
    
    bzero((char *)&__HM_addr, sizeof(__HM_addr));

    __HM_addr.sin_family = AF_INET;

    /* Set up local loopback address for HostManager */
    addr[0] = 127;
    addr[1] = 0;
    addr[2] = 0;
    addr[3] = 1;

    hmserv = (struct servent *)getservbyname(HM_SVCNAME, "udp");
    if (!hmserv)
	return (ZERR_HMPORT);

    __HM_addr.sin_port = hmserv->s_port;

    bcopy(addr, (char *)&__HM_addr.sin_addr, 4);

    __HM_set = 0;

#ifdef KERBEROS    
    if ((krbval = krb_get_lrealm(__Zephyr_realm, 1)) != KSUCCESS)
	return (krbval);
#else
    (void) strcpy(__Zephyr_realm, KRB_REALM);
#endif

    /* Get the sender so we can cache it */
    (void) ZGetSender();

    /* Initialize the input queue */
    __Q_Tail = NULL;
    __Q_Head = NULL;
    
    return (ZERR_NONE);
}