#ifndef __ZSERVER_H__
#define __ZSERVER_H__
/* This file is part of the Project Athena Zephyr Notification System.
 * It contains declarations for use in the server.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /srv/kcr/locker/zephyr/server/zserver.h,v $
 *	$Author: jtkohl $
 *	$Header: /srv/kcr/locker/zephyr/server/zserver.h,v 1.3 1987-07-01 04:15:34 jtkohl Exp $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr.h>		/* which includes <errno.h>,
					   <sys/types.h>,
					   <netinet/in.h>,
					   <sys/time.h>, 
					   <stdio.h> */
#include <arpa/inet.h>
#include <zephyr/acl.h>

#include <syslog.h>
#include "timer.h"
#include "zsrv_err.h"

/* definitions for the Zephyr server */

/* structures */
typedef struct _ZSubscr_t {
	struct _ZSubscr_t *q_forw;	/* links in client's subscr. queue */
	struct _ZSubscr_t *q_back;
	char *zst_class;		/* class of messages */
	char *zst_classinst;		/* class-inst of messages */
	char *zst_recipient;		/* recipient of messages */
} ZSubscr_t;

typedef struct _ZClient_t {
	struct sockaddr_in zct_sin;	/* ipaddr/port of client */
	struct _ZSubscr_t *zct_subs;	/* subscriptions */
	C_Block zct_cblock;		/* session key for this client */
} ZClient_t;

typedef struct _ZClientList_t {
	struct	_ZClientList_t *q_forw;
	struct	_ZClientList_t *q_back;
	ZClient_t	*zclt_client;
} ZClientList_t;

typedef struct _ZAcl_t {
	char *acl_filename;
} ZAcl_t;

typedef	enum _ZAccess_t {
	TRANSMIT,			/* use transmission acl */
	SUBSCRIBE			/* use subscription acl */
} ZAccess_t;

typedef struct _ZClass_t {
	struct	_ZClass_t *q_forw;
	struct	_ZClass_t *q_back;
	char	*zct_classname;
	ZAcl_t	*zct_acl;
	ZClientList_t	*zct_clientlist;
} ZClass_t;

typedef struct _ZHMClient_t {		/* host manager */
	struct	_ZHMClient_t *q_forw;
	struct	_ZHMClient_t *q_back;
	struct	sockaddr_in zhmct_sin;
	long	zhmct_nexttime;		/* time of next keepalive */
} ZHMClient_t;

typedef struct _ZHostList_t {
	struct _ZHostList_t *q_forw;
	struct _ZHostList_t *q_back;
	struct _ZClientList_t *zh_clients;
	struct sockaddr_in zh_addr;	/* IP addr/port of hostmanager */
} ZHostList_t;

typedef enum _server_state {
	SERV_UP,			/* Server is up */
	SERV_TARDY,			/* Server due for a hello */
	SERV_DEAD,			/* Server is considered dead */
	SERV_STARTING			/* Server is between dead and up */
} server_state;

typedef struct _ZServerDesc_t {
	server_state zs_state;		/* server's state */
	struct sockaddr_in zs_addr;	/* server's address */
	long zs_timeout;		/* Length of timeout in sec */
	timer zs_timer;			/* timer struct for this server */
	int zs_numsent;			/* number of hello's sent */
	ZHostList_t *zs_hosts;		/* pointer to list of info from this
					   server */
} ZServerDesc_t;

typedef struct _ZNotAcked_t {
	struct _ZNotAcked_t *q_forw;	/* link to next */
	struct _ZNotAcked_t *q_back;	/* link to prev */
	timer na_timer;			/* timer for retransmit */
	long na_abstimo;		/* absolute timeout to drop after */
	int na_rexmits;			/* number of retransmits */
	caddr_t na_packet;		/* ptr to packet */
	int na_packsz;			/* size of packet */
	ZUnique_Id_t na_uid;		/* uid of packet */
	ZClient_t *na_client;		/* address to send to */
} ZNotAcked_t;

typedef enum _ZSentType {
	NOT_SENT,			/* message was not xmitted */
	SENT,				/* message was xmitted */
	AUTH_FAILED,			/* authentication failed */
	NOT_FOUND			/* user not found for uloc */
} ZSentType;
/* this is just for lint */
struct qelem {
	struct qelem *q_forw;
	struct qelem *q_back;
	char *q_data;
};
/* Function declarations */
	
/* found in access_s.c */
extern int access_check();

/* found in brain_dump.c */
extern void get_brain_dump(), send_brain_dump();

/* found in class_s.c */
extern Code_t class_register(), class_deregister();
extern ZClientList_t *class_lookup();
extern ZAcl_t *class_get_acl();
extern int class_is_control(), class_is_admin(), class_is_hm(), class_is_uloc();

/* found in client_s.c */
extern Code_t client_register();
extern void client_deregister();
extern ZClient_t *client_which_client();

/* found in common.c */
extern char *strsave();

/* found in dispatch.c */
extern void dispatch(), clt_ack(), nack_release(), sendit();

/* found in hostm_s.c */
extern void hostm_dispatch(), hostm_flush(), hostm_shutdown();
extern ZHostList_t *hostm_find_host();
extern ZServerDesc_t *hostm_find_server();

/* found in server_s.c */
extern void server_timo(), server_dispatch(), server_recover();
Code_t server_register();
ZServerDesc_t *server_owner();

/* found in subscr_s.c */
extern Code_t subscr_cancel(), subscr_subscribe();
extern ZClientList_t * subscr_match_list();
extern void subscr_list_free(), subscr_cancel_client();

/* found in uloc_s.c */
extern void ulogin_dispatch(), ulocate_dispatch(), uloc_hflush();

/* found in zctl.c */
extern void control_dispatch();

/* found in libc.a */
char *malloc(), *realloc();

/* global identifiers */

/* found in main.c */
extern struct in_addr my_addr;		/* my inet address */
extern struct sockaddr_in sock_sin;	/* socket descriptors */
extern int srv_socket;			/* dgram sockets for clients
					   and other servers */
extern int zdebug;
extern char myname[];			/* domain name of this host */
extern ZServerDesc_t *otherservers;	/* array of servers */
extern int me_server_idx;		/* me (in the array of servers) */
extern int nservers;			/* number of other servers*/
extern ZNotAcked_t *nacklist;		/* list of not ack'ed packets */

/* useful defines */

#define	REXMIT_SECS	((long) 10)	/* rexmit delay on normal notices */
#define	NUM_REXMITS	(5)		/* number of rexmits */

#define	TIMO_UP		((long) 10)	/* timeout between up and tardy */
#define	TIMO_TARDY	((long) 30)	/* timeout btw tardy hellos */
#define	TIMO_DEAD	((long)(15*60))	/* timeout between hello's for dead */

#define	H_NUM_TARDY	5		/* num hello's before going dead
					   when tardy */
#define	H_NUM_STARTING	2		/* num hello's before going dead
					   when starting */

#define	NULLZCT		((ZClass_t *) 0)
#define	NULLZCNT	((ZClient_t *) 0)
#define	NULLZCLT	((ZClientList_t *) 0)
#define	NULLHMCT	((ZHMClient_t *) 0)
#define	NULLZST		((ZSubscr_t *) 0)
#define	NULLZHLT	((ZHostList_t *) 0)
#define	NULLZNAT	((ZNotAcked_t *) 0)
#define	NULLZACLT	((ZAcl_t *) 0)
#define	NULLZPT		((ZPacket_t *) 0)
#define	NULLZSDT	((ZServerDesc_t *) 0)

#define	me_server	&otherservers[me_server_idx]

#define	ack(a,b)	clt_ack(a,b,SENT)
#define	nack(a,b)	clt_ack(a,b,NOT_SENT)

/* these are to keep lint happy */
#define	xfree(foo)	free((caddr_t) (foo))
#define	xinsque(a,b)	insque((struct qelem *)(a), (struct qelem *)(b))
#define xremque(a)	remque((struct qelem *)(a))
#define	xmalloc(a)	malloc((unsigned)(a))

/* the magic class to match all packets */
#define	MATCHALL_CLASS	"ZMATCH_ALL"

/* ACL's for pre-registered classes */
#define	ZEPHYR_CTL_ACL	"/usr/athena/lib/zephyr/zctl.acl"
#define	HM_ACL		"/usr/athena/lib/zephyr/hm.acl"
#define	LOGIN_ACL	"/usr/athena/lib/zephyr/login.acl"
#define	LOCATE_ACL	"/usr/athena/lib/zephyr/locate.acl"
#define	MATCH_ALL_ACL	"/usr/athena/lib/zephyr/matchall.acl"

/* debugging macros */
#ifdef DEBUG
#define zdbug1(s1)	if (zdebug) syslog(LOG_DEBUG, s1);
#define zdbug2(s1,s2)	if (zdebug) syslog(LOG_DEBUG, s1, s2);
#define zdbug3(s1,s2,s3)	if (zdebug) syslog(LOG_DEBUG, s1, s2, s3);
#else !DEBUG
#define zdbug1(s1)
#define zdbug2(s1,s2)
#define zdbug3(s1,s2,s3)
#endif DEBUG

#endif !__ZSERVER_H__
