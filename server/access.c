/* This file is part of the Project Athena Zephyr Notification System.
 * It contains functions for dealing with acl's.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /srv/kcr/locker/zephyr/server/access.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#ifndef lint
#ifndef SABER
static char rcsid_acl_s_c[] = "$Header: /srv/kcr/locker/zephyr/server/access.c,v 1.3 1987-07-07 13:55:34 jtkohl Exp $";
#endif SABER
#endif lint

/*
 *
 * External routines:
 *
 * int access_check(notice, acl, accesstype)
 *	ZNotice_t *notice;
 *	ZAcl_t *acl;
 *	ZAccess_t accesstype;
 */

/*
 * Each restricted class has two ACL's associated with it, one
 * governing subscriptions and one governing transmission.
 * This module provides the 'glue' between the standard Athena ACL
 * routines and the support needed by the Zephyr server.
 */

#include "zserver.h"
#include <sys/param.h>

/*
 * check access.  return 1 if ok, 0 if not ok.
 */

int
access_check(notice, acl, accesstype)
ZNotice_t *notice;
ZAcl_t *acl;
ZAccess_t accesstype;
{
	char buf[MAXPATHLEN];		/* holds the real acl name */

	if (accesstype != TRANSMIT && accesstype != SUBSCRIBE) {
		syslog(LOG_ERR, "unknown access type %d", accesstype);
		return(0);
	}
	(void) sprintf(buf, "%s.%s", acl->acl_filename,
		(accesstype == TRANSMIT) ? "xmt" : "sub");

	return(acl_check(buf, notice->z_sender));
}
