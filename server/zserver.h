#ifndef __ZSERVER_H__
#define __ZSERVER_H__
/* This file is part of the Project Athena Zephyr Notification System.
 * It contains declarations for use in the server.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /srv/kcr/locker/zephyr/server/zserver.h,v $
 *	$Author: raeburn $
 *	$Header: /srv/kcr/locker/zephyr/server/zserver.h,v 1.30 1988-10-19 22:46:22 raeburn Exp $
 *
 *	Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr.h>		/* which includes <errno.h>,
					   <sys/types.h>,
					   <netinet/in.h>,
					   <sys/time.h>, 
					   <stdio.h>,
					   <krb.h> */
#include <arpa/inet.h>
#include <zephyr/acl.h>
#include <sys/file.h>

#include <zephyr/zsyslog.h>
#include <strings.h>
#include <signal.h>
#ifdef lint
#include <sys/uio.h>			/* so it shuts up about struct iovec */
#endif /* lint */
#include "timer.h"
#include "zsrv_err.h"
#include "zsrv_conf.h"			/* configuration params */

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
#ifdef KERBEROS
	C_Block zct_cblock;		/* session key for this client */
#endif /* KERBEROS */
	char	*zct_principal;		/* krb principal of user */
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
	SUBSCRIBE,			/* use subscription acl */
	INSTWILD,			/* use instance wildcard acl */
	INSTUID				/* use instance UID identity acl */
} ZAccess_t;

typedef struct _ZClass_t {
	struct	_ZClass_t *q_forw;
	struct	_ZClass_t *q_back;
	char	*zct_classname;
	ZAcl_t	*zct_acl;
	ZClientList_t	*zct_clientlist;
} ZClass_t;

typedef struct _ZHostList_t {
	struct _ZHostList_t *q_forw;
	struct _ZHostList_t *q_back;
	struct _ZClientList_t *zh_clients;
	struct sockaddr_in zh_addr;	/* IP addr/port of hostmanager */
	int zh_locked;			/* 1 if this host is locked for
					   a braindump */
} ZHostList_t;

typedef enum _server_state {
	SERV_UP,			/* Server is up */
	SERV_TARDY,			/* Server due for a hello */
	SERV_DEAD,			/* Server is considered dead */
	SERV_STARTING			/* Server is between dead and up */
} server_state;

typedef struct _ZNotAcked_t {
	struct _ZNotAcked_t *q_forw;	/* link to next */
	struct _ZNotAcked_t *q_back;	/* link to prev */
	timer na_timer;			/* timer for retransmit */
	long na_abstimo;		/* absolute timeout to drop after */
	int na_rexmits;			/* number of retransmits */
	caddr_t na_packet;		/* ptr to packet */
	int na_packsz;			/* size of packet */
	ZUnique_Id_t na_uid;		/* uid of packet */
	union {				/* address to send to */
		struct sockaddr_in na_sin; /* client address */
		int srv_idx;		/* index of server */
	} dest;
#define na_addr	dest.na_sin
#define na_srv_idx	dest.srv_idx
} ZNotAcked_t;

typedef struct _ZSrvPending_t {
	struct _ZSrvPending_t *q_forw;	/* link to next */
	struct _ZSrvPending_t *q_back;	/* link to prev */
	caddr_t pend_packet;		/* the notice (in pkt form) */
	int pend_len;			/* len of pkt */
	int pend_auth;			/* whether it is authentic */
	struct sockaddr_in pend_who;	/* the addr of the sender */
} ZSrvPending_t;

typedef struct _ZServerDesc_t {
	server_state zs_state;		/* server's state */
	struct sockaddr_in zs_addr;	/* server's address */
	long zs_timeout;		/* Length of timeout in sec */
	timer zs_timer;			/* timer struct for this server */
	int zs_numsent;			/* number of hello's sent */
	ZHostList_t *zs_hosts;		/* pointer to list of info from this
					   server */
	ZSrvPending_t *zs_update_queue;	/* queue of packets to send
					   to this server when done dumping */
	int zs_dumping;			/* 1 if dumping, so we should queue */
} ZServerDesc_t;

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
	
/* found in access.c */
extern int access_check();
extern void access_init(), access_reinit();

/* found in brain_dump.c */
extern void bdump_get(), bdump_send(), bdump_offer();
extern Code_t bdump_send_list_tcp();

/* found in class.c */
extern Code_t class_register(), class_deregister(), class_restrict();
extern Code_t class_setup_restricted();
extern ZClientList_t *class_lookup();
extern ZAcl_t *class_get_acl();
extern int class_is_control(), class_is_admin(), class_is_hm();
extern int class_is_ulogin(), class_is_uloc();
extern void class_free();

/* found in client.c */
extern Code_t client_register();
extern void client_deregister(), client_dump_clients();
extern ZClient_t *client_which_client();

/* found in common.c */
extern char *strsave();

/* found in dispatch.c */
extern void handle_packet(), dispatch(), clt_ack(), nack_release(), sendit();
extern void xmit();
extern Code_t control_dispatch(), xmit_frag();

/* found in hostm.c */
extern void hostm_flush(), hostm_shutdown(), hostm_losing();
extern ZHostList_t *hostm_find_host();
extern ZServerDesc_t *hostm_find_server();
extern void hostm_transfer(), hostm_deathgram(), hostm_dump_hosts();
extern Code_t hostm_dispatch();
extern void hostm_lose_ignore();

/* found in server.c */
extern void server_timo(), server_recover(), server_dump_servers();
extern void server_init(), server_shutdown();
extern void server_forward(), server_kill_clt(), server_pending_free();
extern void server_self_queue(), server_send_queue(), server_reset();
extern int is_server();
extern ZServerDesc_t *server_which_server();
extern ZSrvPending_t *server_dequeue();
extern Code_t server_dispatch(), server_adispatch();


/* found in subscr.c */
extern Code_t subscr_cancel(), subscr_subscribe(), subscr_send_subs();;
extern ZClientList_t *subscr_match_list();
extern void subscr_free_list(), subscr_cancel_client(), subscr_sendlist();
extern void subscr_dump_subs(), subscr_reset();
extern Code_t subscr_def_subs();

/* found in uloc.c */
extern void uloc_hflush(), uloc_flush_client(), uloc_dump_locs();
extern Code_t ulogin_dispatch(), ulocate_dispatch(), uloc_send_locations();

/* found in libc.a */
char *malloc(), *realloc();
long random();

/* From the Error table library */
char *error_message();

/* global identifiers */

/* found in main.c */
extern struct in_addr my_addr;		/* my inet address */
extern struct sockaddr_in sock_sin;	/* socket descriptors */
extern u_short hm_port;			/* port # of hostmanagers */
extern int srv_socket;			/* dgram sockets for clients
					   and other servers */
extern int bdump_socket;		/* brain dump socket
					   (closed most of the time) */
extern struct sockaddr_in bdump_sin;	/* addr of brain dump socket */

extern fd_set interesting;		/* the file descrips we are listening
					 to right now */
extern int nfildes;			/* number to look at in select() */
extern int zdebug;
extern char myname[];			/* domain name of this host */
extern ZNotAcked_t *nacklist;		/* list of not ack'ed packets */
extern char version[];
extern u_long npackets;			/* num of packets processed */
extern long uptime;			/* time we started */

/* found in bdump.c */
extern int bdumping;			/* are we dumping right now? */

/* found in server.c */
extern ZServerDesc_t *otherservers;	/* array of servers */
extern int me_server_idx;		/* me (in the array of servers) */
extern int nservers;			/* number of other servers*/

#ifdef DEBUG
/* found in dispatch.c */
extern char *pktypes[];			/* names of the packet types */
#endif /* DEBUG */


#define	ADMIN_HELLO	"HELLO"		/* Opcode: hello, are you there */
#define	ADMIN_IMHERE	"IHEARDYOU"	/* Opcode: yes, I am here */
#define	ADMIN_SHUTDOWN	"GOODBYE"	/* Opcode: I am shutting down */
#define ADMIN_BDUMP	"DUMP_AVAIL"	/* Opcode: I will give you a dump */
#define	ADMIN_DONE	"DUMP_DONE"	/* Opcode: brain dump for this server
					   is complete */
#define	ADMIN_NEWCLT	"NEXT_CLIENT"	/* Opcode: this is a new client */
#define	ADMIN_LOST_CLT	"LOST_CLIENT"	/* Opcode: client not ack'ing */
#define	ADMIN_KILL_CLT	"KILL_CLIENT"	/* Opcode: client is dead, remove */
#define	ADMIN_STATUS	"STATUS"	/* Opcode: please send status */

#define	ADMIN_LIMBO	"LIMBO"		/* Class inst: please send limbo info*/
#define	ADMIN_YOU	"YOUR_STATE"	/* Class inst: please send your state*/
#define	ADMIN_ME	"MY_STATE"	/* Class inst: please send my info */

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
#define	NULLZSPT	((ZSrvPending_t *) 0)

/* me_server_idx is the index into otherservers of this server descriptor. */
/* the 'limbo' server is always the first server */

#define	me_server	(&otherservers[me_server_idx])
#define	limbo_server_idx()	(0)
#define	limbo_server	(&otherservers[limbo_server_idx()])

#define	msgs_queued()	(ZQLength() || otherservers[me_server_idx].zs_update_queue)

#define	ack(a,b)	clt_ack(a,b,SENT)
#define	nack(a,b)	clt_ack(a,b,NOT_SENT)

#define	max(a,b)	((a) > (b) ? (a) : (b))

/* these are to keep lint happy */
#define	xfree(foo)	free((caddr_t) (foo))
#define	xinsque(a,b)	insque((struct qelem *)(a), (struct qelem *)(b))
#define xremque(a)	remque((struct qelem *)(a))
#define	xmalloc(a)	malloc((unsigned)(a))

/* the magic class to match all packets */
#define	MATCHALL_CLASS	"zmatch_all"
/* the instance that matches all instances */
#define	WILDCARD_INSTANCE	"*"

/* SERVER_SRVTAB is defined in zephyr.h */
#define	ZEPHYR_SRVTAB	SERVER_SRVTAB

/* debugging macros */
#ifdef DEBUG
#define zdbug(s1)	if (zdebug) syslog s1;
#else /* !DEBUG */
#define zdbug(s1)
#endif /* DEBUG */

#endif /* !__ZSERVER_H__ */
