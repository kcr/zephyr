/* This file is part of the Project Athena Zephyr Notification System.
 * It contains definitions to deal with old 4.2-style syslog functions.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /srv/kcr/locker/zephyr/h/zephyr/Attic/zsyslog.h,v $
 *	$Author: raeburn $
 *
 *	Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/h/zephyr/Attic/zsyslog.h,v 1.1 1988-10-19 16:49:00 raeburn Exp $ */

#include <syslog.h>
#ifndef LOG_AUTH
/* Probably a 4.2-type syslog */
#define	OPENLOG(str, opts, facility)	openlog(str, opts)
#else
/* A decent syslog */
#define OPENLOG(str, opts, facility) openlog(str, opts, facility)
#endif
