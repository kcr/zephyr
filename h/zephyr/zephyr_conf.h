/* This file is part of the Project Athena Zephyr Notification System.
 * It contains configuration definitions.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /srv/kcr/locker/zephyr/h/zephyr/Attic/zephyr_conf.h,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#ifndef __ZEPHYR_CONF_H__
#define __ZEPHYR_CONF_H__

#include <zephyr/mit-copyright.h>

/* Kerberos information */
#define SERVER_SERVICE		"zephyr"
#define SERVER_INSTANCE		"zephyr"
#define SERVER_SRVTAB		"/usr/athena/lib/zephyr/srvtab"

/* General filenames */
#define DEFAULT_VARS_FILE	"/etc/athena/zephyr.vars"

#ifndef KERBEROS
#define	REALM_SZ	MAXHOSTNAMELEN
#endif /* !KERBEROS */

#endif /* __ZEPHYR_CONF_H__ */
