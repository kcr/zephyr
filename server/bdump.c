/* This file is part of the Project Athena Zephyr Notification System.
 * It contains functions for dumping server state between servers.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /srv/kcr/athena/zephyr/server/bdump.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#ifndef lint
#ifndef SABER
static char rcsid_bdump_s_c[] = "$Header: /srv/kcr/athena/zephyr/server/bdump.c,v 1.7 1987-07-16 16:47:31 jtkohl Exp $";
#endif SABER
#endif lint

#include "zserver.h"
#include <sys/socket.h>

static void close_bdump();
static Code_t bdump_send_loop(), bdump_ask_for(), bdump_recv_loop();
static Code_t get_packet(), extract_sin(), send_done(), send_list();
static Code_t send_host_register(), sbd_loop(), gbd_loop(), send_normal_tcp();

static timer bdump_timer;

/*
 * External functions are:
 *
 * void bdump_offer(who)
 *	strut sockaddr_in *who;
 *
 * void bdump_send()
 *
 * void bdump_get(notice, auth, who, server)
 *	ZNotice_t *notice;
 *	int auth;
 *	struct sockaddr_in *who;
 *	ZServerDesc_t *server;
 *
 * Code_t bdump_send_list_tcp(kind, port, class, inst, opcode,
 *			    sender, recip, lyst, num)
 *	ZNotice_Kind_t kind;
 *	u_short port;
 *	char *class, *inst, *opcode, *sender, *recip;
 *	char *lyst[];
 *	int num;
 */
 
/*
 * Functions for performing a brain dump between servers.
 */

/*
 * offer the brain dump to another server
 */

void
bdump_offer(who)
struct sockaddr_in *who;
{
	Code_t retval;
	char buf[512], *addr, *lyst[2];

	if ((bdump_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		syslog(LOG_ERR,"bdump socket: %m");
		bdump_socket = 0;
		return;
	}
	bzero((caddr_t) &bdump_sin, sizeof(bdump_sin));
	bdump_sin.sin_port = htons((u_short)((getpid()*8)&0xfff)+(((int)random()>>4)&0xf)+1024);
	bdump_sin.sin_addr = my_addr;
	bdump_sin.sin_family = AF_INET;
	do {
		if ((retval = bind(bdump_socket, (struct sockaddr *) &bdump_sin, sizeof(bdump_sin))) < 0) {
			if (errno == EADDRINUSE)
				bdump_sin.sin_port = htons(ntohs(bdump_sin.sin_port) + 1);
			else {
				syslog(LOG_ERR, "bdump bind %d: %m", htons(bdump_sin.sin_port));
				(void) close(bdump_socket);
				bdump_socket = 0;
				return;
			}
		}
	} while (retval < 0);

	(void) listen(bdump_socket, 1);

	bdump_timer = timer_set_rel(20L, close_bdump, (caddr_t) 0);
	FD_SET(bdump_socket, &interesting);
	nfildes = max(bdump_socket, srv_socket) + 1;


	addr = inet_ntoa(bdump_sin.sin_addr);
	(void) sprintf(buf, "%d", ntohs(bdump_sin.sin_port));
	lyst[0] = addr;
	lyst[1] = buf;

	if ((retval = ZSetDestAddr(who)) != ZERR_NONE) {
		syslog(LOG_WARNING, "obd set addr: %s",
		       error_message(retval));
		return;
	}

	/* myname is the hostname */
	send_list(ACKED, sock_sin.sin_port, ZEPHYR_ADMIN_CLASS, "",
		  ADMIN_BDUMP, myname, "", lyst, 2);
	
	return;
}

/*
 * Accept a connection, and send the brain dump to the other server
 */

void
bdump_send()
{
	int sock;
	struct sockaddr_in from;
	ZServerDesc_t *server;
	Code_t retval;
	int fromlen = sizeof(from);

	zdbug((LOG_DEBUG, "bdump_send"));
	/* accept the connection, and send the brain dump */
	sock = accept(bdump_socket, &from, &fromlen);

	from.sin_port = sock_sin.sin_port; /* we don't care what port
					    it came from, and we need to
					    fake out server_which_server() */
	server = server_which_server(&from);
	if (!server) {
		syslog(LOG_ERR,"unknown server?");
		server = limbo_server;
	}
	zdbug((LOG_DEBUG, "sbd connected"));

	if (bdump_socket) {
		/* shut down the listening socket and the timer */
		FD_CLR(bdump_socket, &interesting);
		(void) close(bdump_socket);
		nfildes = srv_socket + 1;
		bdump_socket = 0;
		timer_reset(bdump_timer);
	}

	/* Now begin the brain dump.  Set the Zephyr port to be the
	   TCP connection.  This will work since recvfrom and sendto
	   work even on TCP connections, but the sockaddr_in addresses
	   are ignored */
	(void) ZSetFD(sock);
#ifdef notdef
	/* receive the authenticator */
	/* mutually authenticate */
#endif notdef

	if ((retval = sbd_loop(&from)) != ZERR_NONE) {
		syslog(LOG_WARNING, "sbd_loop failed: %s",
		       error_message(retval));
	} else {
		if ((retval = gbd_loop(server)) != ZERR_NONE) {
			syslog(LOG_WARNING, "gbd_loop failed: %s",
			       error_message(retval));
		} else {
			zdbug((LOG_DEBUG, "sbd finished"));
			if (server != limbo_server) {
				/* set this guy to be up,
				   and schedule a hello */
				server->zs_state = SERV_UP;
				timer_reset(server->zs_timer);
				server->zs_timer = timer_set_rel(0L, server_timo, (caddr_t) server);
			}
		}
	}
	(void) ZSetFD(srv_socket);
	(void) close(sock);
	return;
}

/*ARGSUSED*/
void
bdump_get(notice, auth, who, server)
ZNotice_t *notice;
int auth;
struct sockaddr_in *who;
ZServerDesc_t *server;
{
	struct sockaddr_in target;
	int sock;
	Code_t retval;

	zdbug((LOG_DEBUG, "bdump avail"));

	if ((retval = extract_sin(notice, &target)) != ZERR_NONE) {
		syslog(LOG_ERR, "gbd sin: %s", error_message(retval));
		return;
	}
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		syslog(LOG_ERR, "gbd socket: %m");
		return;
	}
	if (connect(sock, &target, sizeof(target))) {
		syslog(LOG_ERR, "gbd connect: %m");
		(void) close(sock);
		return;
	}
	zdbug((LOG_DEBUG, "gbd connected"));

	/* Now begin the brain dump.  Set the Zephyr port to be the
	   TCP connection.  This will work since recvfrom and sendto
	   work even on TCP connections, but the sockaddr_in addresses
	   are ignored */
	(void) ZSetFD(sock);

#ifdef notdef
	/* send the authenticator over */
	/* and receive mutual authent */
#endif notdef
	if ((retval = gbd_loop(server)) != ZERR_NONE) {
		syslog(LOG_WARNING, "gbd_loop failed: %s",
		       error_message(retval));
	} else {
		if ((retval = sbd_loop(&target)) != ZERR_NONE) {
			syslog(LOG_WARNING, "sbd_loop failed: %s",
			       error_message(retval));
		} else {
			zdbug((LOG_DEBUG, "gbd finished"));
			/* set this guy to be up,
			   and schedule a hello */
			server->zs_state = SERV_UP;
			timer_reset(server->zs_timer);
			server->zs_timer = timer_set_rel(0L, server_timo, (caddr_t) server);
		}
	}

	(void) ZSetFD(srv_socket);
	(void) close(sock);
	return;
}

/*
 * Send a list off as the specified notice
 */

Code_t
bdump_send_list_tcp(kind, port, class, inst, opcode, sender, recip, lyst, num)
ZNotice_Kind_t kind;
u_short port;
char *class, *inst, *opcode, *sender, *recip;
char *lyst[];
int num;
{
	ZNotice_t notice;
	register ZNotice_t *pnotice; /* speed hack */
	ZPacket_t pack;
	int packlen, count;
	Code_t retval;
	u_short length;

	pnotice = &notice;

	pnotice->z_kind = kind;

	pnotice->z_port = port;
	pnotice->z_class = class;
	pnotice->z_class_inst = inst;
	pnotice->z_opcode = opcode;
	pnotice->z_sender = sender;
	pnotice->z_recipient = recip;

	packlen = sizeof(pack);
	
	if ((retval = ZFormatNoticeList(pnotice, lyst, num, pack, packlen, &packlen, ZNOAUTH)) != ZERR_NONE)
		return(retval);
	
	length = htons((u_short) packlen);

	if ((count = write(ZGetFD(), (caddr_t) &length, sizeof(length))) != sizeof(length))
		if (count < 0)
			return(count);
		else {
			syslog(LOG_WARNING, "slt xmit: %d vs %d",sizeof(length),count);
			return(ZSRV_PKSHORT);
		}

	if ((count = write(ZGetFD(), pack, packlen)) != packlen)
		if (count < 0)
			return(count);
		else {
			syslog(LOG_WARNING, "slt xmit: %d vs %d",packlen, count);
			return(ZSRV_PKSHORT);
		}
	return(ZERR_NONE);
}

static Code_t
sbd_loop(from)
struct sockaddr_in *from;
{
	ZNotice_t bd_notice;
	ZPacket_t pack;
	int packlen = sizeof(pack);
	Code_t retval;
	int auth;

	while (1) {
		packlen = sizeof(pack);
		if ((retval = get_packet(pack, packlen, &packlen)) != ZERR_NONE) {
			syslog(LOG_ERR, "sbd notice get: %s",
			       error_message(retval));
			return(retval);
		}
		if ((retval = ZParseNotice(pack, packlen, &bd_notice, &auth, from)) != ZERR_NONE) {
			syslog(LOG_ERR, "sbd notice parse: %s",
			       error_message(retval));
			return(retval);
		}
#ifdef DEBUG
		if (zdebug) {
			char buf[4096];
		
			(void) sprintf(buf, "bdump:%s '%s' '%s' '%s' '%s' '%s'",
				       pktypes[(int) bd_notice.z_kind],
				       bd_notice.z_class,
				       bd_notice.z_class_inst,
				       bd_notice.z_opcode,
				       bd_notice.z_sender,
				       bd_notice.z_recipient);
			syslog(LOG_DEBUG, buf);
		}
#endif DEBUG
		if (!strcmp(bd_notice.z_class_inst, ADMIN_LIMBO)) {
			/* he wants limbo */
			zdbug((LOG_DEBUG, "limbo req"));
			if ((retval = bdump_send_loop(limbo_server)) != ZERR_NONE)
				return(retval);
			continue;
		} else if (!strcmp(bd_notice.z_class_inst, ADMIN_YOU)) {
			/* he wants my state */
			zdbug((LOG_DEBUG, "my state req"));
			if ((retval = bdump_send_loop(me_server)) != ZERR_NONE)
				return(retval);
			break;
		} else if (!strcmp(bd_notice.z_class_inst, ADMIN_DONE)) {
			break;
		} else {
			/* what does he want? */
			zdbug((LOG_DEBUG, "unknown req"));
			break;
		}
	}
	return(ZERR_NONE);
}

static Code_t
gbd_loop(server)
ZServerDesc_t *server;
{
	struct sockaddr_in target;
	Code_t retval;

	/* if we have no hosts in the 'limbo' state (on the limbo server),
	   ask for the other server to send us the limbo state. */
	if (otherservers[limbo_server_idx()].zs_hosts->q_forw ==
	    otherservers[limbo_server_idx()].zs_hosts) {
		if ((retval = bdump_ask_for("LIMBO")) != ZERR_NONE)
			return(retval);
		if ((retval = bdump_recv_loop(&otherservers[limbo_server_idx()], &target)) != ZERR_NONE)
			return(retval);
	}

	if ((retval = bdump_ask_for("YOUR_STATE")) != ZERR_NONE)
		return(retval);
	retval = bdump_recv_loop(server, &target);
	return(retval);
}
/*
 * The braindump offer wasn't taken, so we retract it.
 */

/*ARGSUSED*/
static void
close_bdump(arg)
caddr_t arg;
{
	if (bdump_socket) {
		FD_CLR(bdump_socket, &interesting);
		(void) close(bdump_socket);
		nfildes = srv_socket + 1;
		bdump_socket = 0;
		zdbug((LOG_DEBUG, "bdump not used"));
	} else {
		zdbug((LOG_DEBUG, "bdump not open"));
	}
	return;
}

/*
 * Ask the other server to send instruction packets for class instance
 * inst
 */

static Code_t
bdump_ask_for(inst)
char *inst;
{
	Code_t retval;

	/* myname is the hostname */
	retval = send_normal_tcp(ACKED, bdump_sin.sin_port, ZEPHYR_ADMIN_CLASS,
				 inst, ADMIN_BDUMP, myname, "",
				 (char *) NULL, 0);
	return(retval);
}

/*
 * Start receiving instruction notices from the brain dump socket
 */

static Code_t
bdump_recv_loop(server, target)
ZServerDesc_t *server;
struct sockaddr_in *target;
{
	ZNotice_t notice;
	ZPacket_t packet;
	int len, auth;
	Code_t retval;
	ZClient_t *client = NULLZCNT;
	struct sockaddr_in current_who;
	int who_valid = 0;
	register char *cp;

	zdbug((LOG_DEBUG, "bdump recv loop"));
	
	/* XXX do the inverse, registering stuff on the fly */
	while (1) {
		len = sizeof(packet);
		if ((retval = get_packet(packet, len, &len)) != ZERR_NONE) {
			syslog(LOG_ERR, "brl get pkt: %s",
			       error_message(retval));
			return(retval);
		}
		if ((retval = ZParseNotice(packet, len, &notice, &auth, target)) != ZERR_NONE) {
			syslog(LOG_ERR, "brl notice parse: %s",
			       error_message(retval));
			return(retval);
		}
#ifdef DEBUG
		if (zdebug) {
			char buf[4096];
		
			(void) sprintf(buf, "bdump:%s '%s' '%s' '%s' '%s' '%s'",
				       pktypes[(int) notice.z_kind],
				       notice.z_class,
				       notice.z_class_inst,
				       notice.z_opcode,
				       notice.z_sender,
				       notice.z_recipient);
			syslog(LOG_DEBUG, buf);
		}
#endif DEBUG
		if (notice.z_kind == HMCTL) {
			/* host register */
			if ((retval = extract_sin(&notice, &current_who)) !=
			    ZERR_NONE) {
				syslog(LOG_ERR, "brl hmctl sin: %s",
				       error_message(retval));
				return(retval);
			}
			who_valid = 1;
			/* 1 = tell it we are authentic */
			hostm_dispatch(&notice, 1, &current_who, server);
		} else if (!strcmp(notice.z_opcode, ADMIN_DONE)) {
			/* end of brain dump */
			return(ZERR_NONE);
		} else if (!who_valid) {
			syslog(LOG_ERR, "brl: no current host");
			return(ZSRV_HNOTFOUND);
		} else if (!strcmp(notice.z_class, LOGIN_CLASS)) {
			/* 1 = tell it we are authentic */
			ulogin_dispatch(&notice, 1, &current_who, server);
		} else if (!strcmp(notice.z_opcode, ADMIN_NEWCLT)) {
			/* register a new client */
			notice.z_port = ntohs((u_short)atoi(notice.z_message));
			if ((retval = client_register(&notice,
						      &current_who,
						      &client,
						      server)) != ZERR_NONE) {
				syslog(LOG_ERR,"brl register failed: %s",
				       error_message(retval));
				return(retval);
			}
			if (strlen(notice.z_message) + 1 < notice.z_message_len) {
				/* a C_Block is there */
				cp = notice.z_message +
					strlen(notice.z_message) + 1;
				if (ZReadAscii(cp,strlen(cp),client->zct_cblock,sizeof(C_Block)) != ZERR_NONE)
					bzero((caddr_t) client->zct_cblock,
					      sizeof(C_Block));
			}
		} else if (!strcmp(notice.z_opcode, CLIENT_SUBSCRIBE)) { 
			/* a subscription packet */
			if (!client) {
				syslog(LOG_ERR, "brl no client");
				return(ZSRV_NOCLT);
			}
			if ((retval = subscr_subscribe(client, &notice)) != ZERR_NONE) {
				syslog(LOG_WARNING, "brl subscr failed: %s",
				       error_message(retval));
				return(retval);
			}
		} else {
			syslog(LOG_ERR, "brl bad opcode %s",notice.z_opcode);
			return(ZSRV_UNKNOWNOPCODE);
			break;
		}
	}
}

/*
 * Send all the state from server to the peer.
 */

static Code_t
bdump_send_loop(server)
register ZServerDesc_t *server;
{
	register ZHostList_t *host;
	register ZClientList_t *clist;
	Code_t retval;

	zdbug((LOG_DEBUG, "bdump send loop"));

	for (host = server->zs_hosts->q_forw;
	     host != server->zs_hosts;
	     host = host->q_forw) {
		/* for each host */
		if ((retval = send_host_register(host)) != ZERR_NONE)
			return(retval);
		if ((retval = uloc_send_locations(host)) != ZERR_NONE)
			return(retval);
		if (!host->zh_clients)
			continue;
		for (clist = host->zh_clients->q_forw;
		     clist != host->zh_clients;
		     clist = clist->q_forw) {
			/* for each client */
			if (!clist->zclt_client->zct_subs)
				continue;
			if ((retval = subscr_send_subs(clist->zclt_client))
			    != ZERR_NONE)
				return(retval);
		}
	}
	retval = send_done();
	return(retval);
}

/*
 * Send a host boot packet to the other server
 */

static Code_t
send_host_register(host)
ZHostList_t *host;
{
	char buf[512], *addr, *lyst[2];
	Code_t retval;

	zdbug((LOG_DEBUG, "bdump_host_register"));
	addr = inet_ntoa(host->zh_addr.sin_addr);
	(void) sprintf(buf, "%d", ntohs(host->zh_addr.sin_port));
	lyst[0] = addr;
	lyst[1] = buf;

	/* myname is the hostname */
	if ((retval = bdump_send_list_tcp(HMCTL, bdump_sin.sin_port, ZEPHYR_CTL_CLASS, ZEPHYR_CTL_HM, HM_BOOT, myname, "", lyst, 2)) != ZERR_NONE)
		syslog(LOG_ERR, "shr send: %s",error_message(retval));
	return(retval);
}

/*
 * Send a sync indicating end of this host
 */

static Code_t
send_done()
{
	Code_t retval;

	zdbug((LOG_DEBUG, "send_done"));
	retval = send_normal_tcp(SERVACK, bdump_sin.sin_port,
				 ZEPHYR_ADMIN_CLASS, "", ADMIN_DONE, myname,
				 "", (char *) NULL, 0);
	return(retval);
}


/*
 * Send a list off as the specified notice
 */

static Code_t
send_list(kind, port, class, inst, opcode, sender, recip, lyst, num)
ZNotice_Kind_t kind;
u_short port;
char *class, *inst, *opcode, *sender, *recip;
char *lyst[];
int num;
{
	ZNotice_t notice;
	register ZNotice_t *pnotice; /* speed hack */
	ZPacket_t pack;
	int packlen;
	Code_t retval;

	pnotice = &notice;

	pnotice->z_kind = kind;

	pnotice->z_port = port;
	pnotice->z_class = class;
	pnotice->z_class_inst = inst;
	pnotice->z_opcode = opcode;
	pnotice->z_sender = sender;
	pnotice->z_recipient = recip;

	packlen = sizeof(pack);
	
	if ((retval = ZFormatNoticeList(pnotice, lyst, num, pack, packlen, &packlen, ZNOAUTH)) != ZERR_NONE) {
		syslog(LOG_WARNING, "sl format: %s", error_message(retval));
		return(retval);
	}
	
	if ((retval = ZSendPacket(pack, packlen)) != ZERR_NONE) {
		syslog(LOG_WARNING, "sl xmit: %s", error_message(retval));
		return(retval);
	}
	return(ZERR_NONE);
}

/*
 * Send a message off as the specified notice, via TCP
 */

static Code_t
send_normal_tcp(kind, port, class, inst, opcode, sender, recip, message, len)
ZNotice_Kind_t kind;
u_short port;
char *class, *inst, *opcode, *sender, *recip;
char *message;
int len;
{
	ZNotice_t notice;
	register ZNotice_t *pnotice; /* speed hack */
	ZPacket_t pack;
	int packlen, count;
	Code_t retval;
	u_short length;

	pnotice = &notice;

	pnotice->z_kind = kind;

	pnotice->z_port = port;
	pnotice->z_class = class;
	pnotice->z_class_inst = inst;
	pnotice->z_opcode = opcode;
	pnotice->z_sender = sender;
	pnotice->z_recipient = recip;
	pnotice->z_message = message;
	pnotice->z_message_len = len;

	packlen = sizeof(pack);
	
	if ((retval = ZFormatNotice(pnotice, pack, packlen, &packlen, ZNOAUTH)) != ZERR_NONE) {
		syslog(LOG_WARNING, "sn format: %s", error_message(retval));
		return(retval);
	}

	length = htons((u_short) packlen);

	if ((count = write(ZGetFD(), (caddr_t) &length, sizeof(length))) != sizeof(length)) {
		if (count < 0) {
			syslog(LOG_WARNING, "snt xmit/len: %m");
			return(errno);
		} else {
			syslog(LOG_WARNING, "snt xmit: %d vs %d",sizeof(length),count);
			return(ZSRV_LEN);
		}
	}

	if ((count = write(ZGetFD(), pack, packlen)) != packlen)
		if (count < 0) {
			syslog(LOG_WARNING, "snt xmit: %m");
			return(errno);
		} else {
			syslog(LOG_WARNING, "snt xmit: %d vs %d",packlen, count);
			return(ZSRV_LEN);
		}
	return(ZERR_NONE);
}

/*
 * get a packet from the TCP socket
 * return 0 if successful, error code else
 */

static Code_t
get_packet(packet, len, retlen)
caddr_t packet;
int len;
int *retlen;				/* RETURN */
{
	u_short length;
	int result;

	if ((result = read(ZGetFD(), (caddr_t) &length, sizeof(u_short))) < sizeof(short)) {
		if (result < 0)
			return(errno);
		else {
			syslog(LOG_ERR, "get_pkt len: %d vs %d", result, sizeof(short));
			return(ZSRV_LEN);
		}
	}
	
	length = ntohs(length);
	if (len < length)
		return(ZSRV_BUFSHORT);
	if ((result = read(ZGetFD(), packet, (int) length)) < length) {
		if (result < 0)
			return(errno);
		else {
			syslog(LOG_ERR, "get_pkt: %d vs %d",result, length);
			return(ZSRV_LEN);
		}
	}
	*retlen = (int) length;
	return(ZERR_NONE);
}

static Code_t
extract_sin(notice, target)
ZNotice_t *notice;
struct sockaddr_in *target;
{
	register char *cp = notice->z_message;
	char *buf;

	buf = cp;
	if (!notice->z_message_len || *buf == '\0') {
		zdbug((LOG_DEBUG,"no addr"));
		return(ZSRV_PKSHORT);
	}
	target->sin_addr.s_addr = inet_addr(cp);
	cp += (strlen(cp) + 1); /* past the null */
	if ((cp >= notice->z_message + notice->z_message_len)
	    || (*cp == '\0')) {
		zdbug((LOG_DEBUG, "no port"));
		return(ZSRV_PKSHORT);
	}
	target->sin_port = ntohs((u_short) atoi(cp));
	target->sin_family = AF_INET;
	return(ZERR_NONE);
}
