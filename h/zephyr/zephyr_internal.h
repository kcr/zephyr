/* This file is part of the Project Athena Zephyr Notification System.
 * It contains internal definitions for the client library.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/h/zephyr/Attic/zephyr_internal.h,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/h/zephyr/Attic/zephyr_internal.h,v 1.8 1987-11-01 17:51:49 rfrench Exp $ */

#ifndef __ZINTERNAL_H__
#define __ZINTERNAL_H__

#include <zephyr/zephyr.h>
#include <strings.h>			/* for strcpy, etc. */
#include <sys/types.h>			/* for time_t, uid_t, etc */

struct _Z_InputQ {
	struct		_Z_InputQ *next;
	struct		_Z_InputQ *prev;
	int		packet_len;
	struct		sockaddr_in from;
	ZPacket_t	packet;
};

extern struct _Z_InputQ *__Q_Head, *__Q_Tail;

extern int __Zephyr_open;
extern int __HM_set;
extern int __Zephyr_server;

extern ZLocations_t *__locate_list;
extern int __locate_num;
extern int __locate_next;

extern ZSubscription_t *__subscriptions_list;
extern int __subscriptions_num;
extern int __subscriptions_next;

extern int krb_err_base;

extern char *malloc();
extern time_t time();
extern long random();

#endif !__ZINTERNAL_H__
