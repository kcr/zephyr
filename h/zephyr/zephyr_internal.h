/* This file is part of the Project Athena Zephyr Notification System.
 * It contains internal definitions for the client library.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/h/zephyr/Attic/zephyr_internal.h,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/h/zephyr/Attic/zephyr_internal.h,v 1.23 1994-11-01 17:50:52 ghudson Exp $ */

#ifndef __ZINTERNAL_H__
#define __ZINTERNAL_H__

#include <zephyr/zephyr.h>
#include <string.h>
#include <sys/types.h>			/* for time_t, uid_t, etc */

#ifdef lint
#include <sys/uio.h>	/* to make lint shut up about struct/union iovec */
#endif

struct _Z_Hole {
    struct _Z_Hole	*next;
    int			first;
    int			last;
};

struct _Z_InputQ {
    struct _Z_InputQ	*next;
    struct _Z_InputQ	*prev;
    ZNotice_Kind_t	kind;
    unsigned long	timep;
    int			packet_len;
    char		*packet;
    int			complete;
    struct sockaddr_in	from;
    struct _Z_Hole	*holelist;
    ZUnique_Id_t	uid;
    int			auth;
    int			header_len;
    char		*header;
    int			msg_len;
    char		*msg;
};

extern struct _Z_InputQ *__Q_Head, *__Q_Tail;

	/* Maximum number of packet fragments */
#define Z_MAXFRAGS		500	/* Probably around 350K */

	/* Maximum allowable size of an incoming notice */
#define Z_MAXNOTICESIZE		400000

	/* Maximum allowable size of all notices in the input queue */
	/* This is more of a ballpark figure than a hard limit */
#define Z_MAXQUEUESIZE		1500000

	/* Amount of room to leave for multinotice field */
#define Z_FRAGFUDGE		13	/* 999999/999999 */

	/* Amount of time a notice can stay in the queue without being
	 * touched by an incoming fragment */
#define Z_NOTICETIMELIMIT	30	/* seconds */

	/* Initial size of old uid buffer. */
#define Z_INITFILTERSIZE		30	/* uid's */

extern int __Zephyr_open; /* 0 if the library opened the FD, 1 otherwise */
extern int __HM_set; /* 0 if the library set the dest addr, 1 otherwise */
extern int __Zephyr_server; /* 0 if normal client, 1 if server */

extern ZLocations_t *__locate_list;
extern int __locate_num;
extern int __locate_next;

extern ZSubscription_t *__subscriptions_list;
extern int __subscriptions_num;
extern int __subscriptions_next;

#ifdef Z_HaveKerberos
extern int krb_err_base;
#endif

extern time_t time Zproto((time_t *));
extern long random();

extern struct _Z_InputQ *Z_GetFirstComplete();
extern struct _Z_InputQ *Z_GetNextComplete();
extern Code_t Z_XmitFragment Zproto((ZNotice_t*, char *,int,int));
extern void Z_RemQueue Zproto ((struct _Z_InputQ *));

#endif /* !__ZINTERNAL_H__ */
