/* This file is part of the Project Athena Zephyr Notification System.
 * It contains functions for dispatching a notice.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /srv/kcr/locker/zephyr/server/dispatch.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#ifndef lint
#ifndef SABER
static char rcsid_dispatch_c[] = "$Header: /srv/kcr/locker/zephyr/server/dispatch.c,v 1.5 1987-07-07 17:17:57 jtkohl Exp $";
#endif SABER
#endif lint

#include "zserver.h"

/*
 *
 * External Routines:
 *
 * void dispatch(notice, auth)
 *	ZNotice_t *notice;
 *	int auth;
 *
 * void clt_ack(notice, who, sent)
 *	ZNotice_t *notice;
 *	struct sockaddr_in *who;
 *	ZSentType sent;
 *
 * void nack_release(client)
 *	ZClient_t *client;
 *
 * void sendit(notice, auth, who)
 *	ZNotice_t *notice;
 *	int auth;
 *	struct sockaddr_in *who;
 */

static void xmit(), rexmit(), nack_cancel();
static int is_server();
#ifdef DEBUG
static void dump_nack();
#endif DEBUG

/* patchable magic numbers controlling the retransmission rate and count */
int num_rexmits = NUM_REXMITS;
long rexmit_secs = REXMIT_SECS;
long abs_timo = REXMIT_SECS*NUM_REXMITS + 10;

#ifdef DEBUG
static char *pktypes[] = {
	"UNSAFE",
	"UNACKED",
	"ACKED",
	"HMACK",
	"HMCTL",
	"SERVACK",
	"SERVNAK",
	"CLIENTACK"
};
#endif DEBUG

/*
 * Dispatch a notice.
 */

void
dispatch(notice, auth, who)
register ZNotice_t *notice;
int auth;
struct sockaddr_in *who;
{

#ifdef DEBUG
	if (zdebug) {
		char buf[4096];
		
		(void) sprintf(buf, "disp:%s '%s' '%s' '%s' '%s' '%s' %s/%d/%d",
			       pktypes[(int) notice->z_kind],
			       notice->z_class,
			       notice->z_class_inst,
			       notice->z_opcode,
			       notice->z_sender,
			       notice->z_recipient,
			       inet_ntoa(who->sin_addr),
			       ntohs(who->sin_port),
			       ntohs(notice->z_port));
		syslog(LOG_DEBUG, buf);
	}
#endif DEBUG
	if (notice->z_kind == CLIENTACK) {
		nack_cancel(notice, who);
		return;
	}
	if (is_server(who)) {
		server_dispatch(notice, auth, who);
		return;
	} else if (class_is_hm(notice)) {
		hostm_dispatch(notice, auth, who);
		return;
	} else if (class_is_control(notice)) {
		control_dispatch(notice, auth, who);
		return;
	} else if (class_is_ulogin(notice)) {
		ulogin_dispatch(notice, auth, who);
		return;
	} else if (class_is_ulocate(notice)) {
		ulocate_dispatch(notice, auth, who);
		return;
	} else if (class_is_admin(notice)) {
		server_adispatch(notice, auth, who);
		return;
	}

	/* oh well, do the dirty work */
	sendit(notice, auth, who);
}

/*
 * Send a notice off to those clients who have subscribed to it.
 */

void
sendit(notice, auth, who)
register ZNotice_t *notice;
int auth;
struct sockaddr_in *who;
{
	int acked = 0;
	ZAcl_t *acl;
	register ZClientList_t *clientlist, *ptr;

	if ((acl = class_get_acl(notice->z_class)) &&
	    (!auth || !access_check(notice, acl, TRANSMIT))) {
		syslog(LOG_WARNING, "sendit unauthorized %s", notice->z_class);
		clt_ack(notice, who, AUTH_FAILED);
		return;
	}	
	if ((clientlist = subscr_match_list(notice, acl))) {
		for (ptr = clientlist->q_forw;
		     ptr != clientlist;
		     ptr = ptr->q_forw) {
			/* for each client who gets this notice,
			   send it along */
			xmit(notice, ptr->zclt_client, auth);
			if (!acked) {
				acked = 1;
				ack(notice, who);
			}
		}
		subscr_list_free(clientlist);
	}

	if (!acked)
		nack(notice, who);
}

/*
 * Clean up the not-yet-acked queue and release anything destined
 * for the client.
 */

void
nack_release(client)
ZClient_t *client;
{
	register ZNotAcked_t *nacked, *nack2;

	/* search the not-yet-acked list for anything destined to him, and
	   flush it. */
	for (nacked = nacklist->q_forw;
	     nacked != nacklist;)
		if (nacked->na_client == client) {
			zdbug((LOG_DEBUG,"nack_rel: punt 0x%x, 0x%x",
			       nacked,
			       nacked->na_timer));
			/* go back, since remque will change things */
			nack2 = nacked->q_back;
			timer_reset(nacked->na_timer);
			xfree(nacked->na_packet);
			xremque(nacked);
			/* now that the remque adjusted the linked list,
			   we go forward again */
			nacked = nack2->q_forw;
		} else
			nacked = nacked->q_forw;
	return;
}

/*
 * Is this from a server?
 */
static int
is_server(who)
struct sockaddr_in *who;
{
	register ZServerDesc_t *servs;
	register int num;

	if (who->sin_port != sock_sin.sin_port)
		return(0);
		
	/* just look over the server list */
	for (servs = otherservers, num = 0; num < nservers; num++, servs++)
		if (servs->zs_addr.sin_addr.s_addr == who->sin_addr.s_addr)
			return(1);
	return(0);
}

/*
 * Send the notice to the client.  After transmitting, put it onto the
 * not ack'ed list.
 */

static void
xmit(notice, client, auth)
register ZNotice_t *notice;
ZClient_t *client;
int auth;
{
	caddr_t noticepack;
	register ZNotAcked_t *nacked;
	int packlen;
	Code_t retval;

	zdbug((LOG_DEBUG,"xmit"));
	if (!(noticepack = (caddr_t) xmalloc(sizeof(ZPacket_t)))) {
		syslog(LOG_ERR, "xmit malloc");
		return;			/* DON'T put on nack list */
	}


	packlen = sizeof(ZPacket_t);

	if (auth) {			/* we are distributing authentic */
		if ((retval = ZFormatAuthenticNotice(notice,
						     noticepack,
						     packlen,
						     &packlen,
						     client->zct_cblock))
		    != ZERR_NONE) {
			syslog(LOG_ERR, "xmit auth format: %s",
			       error_message(retval));
			xfree(noticepack);
			return;
		}
	} else {
		notice->z_auth = 0;
		notice->z_authent_len = 0;
		notice->z_ascii_authent = (char *)"";
		if ((retval = ZFormatRawNotice(notice,
					       noticepack,
					       packlen,
					       &packlen)) != ZERR_NONE) {
			syslog(LOG_ERR, "xmit format: %s",
			       error_message(retval));
			xfree(noticepack);
			return;			/* DON'T put on nack list */
		}
	}
	zdbug((LOG_DEBUG," to %s/%d",inet_ntoa(client->zct_sin.sin_addr),
	       ntohs(client->zct_sin.sin_port)));
	if ((retval = ZSetDestAddr(&client->zct_sin)) != ZERR_NONE) {
		syslog(LOG_WARNING, "xmit set addr: %s",
		       error_message(retval));
		return;
	}
	if ((retval = ZSendPacket(noticepack, packlen)) != ZERR_NONE) {
		syslog(LOG_WARNING, "xmit xmit: %s", error_message(retval));
		return;
	}

	/* now we've sent it, mark it as not ack'ed */

	if (!(nacked = (ZNotAcked_t *)xmalloc(sizeof(ZNotAcked_t)))) {
		/* no space: just punt */
		syslog(LOG_WARNING, "xmit nack malloc");
		return;
	}

	nacked->na_rexmits = 0;
	nacked->na_packet = noticepack;
	nacked->na_client = client;
	nacked->na_packsz = packlen;
	nacked->na_uid = notice->z_uid;
	nacked->q_forw = nacked->q_back = nacked;
	nacked->na_abstimo = NOW + abs_timo;

	/* set a timer to retransmit when done */
	nacked->na_timer = timer_set_rel(rexmit_secs,
					 rexmit,
					 (caddr_t) nacked);
	/* chain in */
	xinsque(nacked, nacklist);
#ifdef DEBUG
	if (zdebug)
		dump_nack();
#endif DEBUG

}

#ifdef DEBUG
/*
 * log the nack queue
 */

static void
dump_nack()
{
	register ZNotAcked_t *nacked;

	   
	if (zdebug)
		for (nacked = nacklist->q_forw;
		     nacked != nacklist;
		     nacked = nacked->q_forw)
			syslog(LOG_DEBUG, "nck 0x%x, tmr 0x%x, clt 0x%x",
			       nacked, nacked->na_timer, nacked->na_client);
}
#endif DEBUG

/*
 * Retransmit the packet specified.  If we have timed out or retransmitted
 * too many times, punt the packet and initiate the host recovery algorithm
 * Else, increment the count and re-send the notice packet.
 */

static void
rexmit(nackpacket)
register ZNotAcked_t *nackpacket;
{
	int retval;

	register ZClient_t *client;
	zdbug((LOG_DEBUG,"rexmit"));

	if (++(nackpacket->na_rexmits) > num_rexmits ||
	    NOW > nackpacket->na_abstimo) {
		/* possibly dead client */

		client = nackpacket->na_client;
		syslog(LOG_WARNING, "lost client %s %d",
		       inet_ntoa(client->zct_sin.sin_addr),
		       ntohs(client->zct_sin.sin_port));

		/* unlink & free packet */
		xremque(nackpacket);
		xfree(nackpacket->na_packet);
		xfree(nackpacket);

#ifdef DEBUG
		if (zdebug)
			dump_nack();
#endif DEBUG
		/* initiate recovery */
		server_recover(client);
		return;
	}

	/* retransmit the packet */
	
	zdbug((LOG_DEBUG," to %s/%d",
	       inet_ntoa(nackpacket->na_client->zct_sin.sin_addr),
	       ntohs(nackpacket->na_client->zct_sin.sin_port)));
	if ((retval = ZSetDestAddr(&nackpacket->na_client->zct_sin))
	    != ZERR_NONE) {
		syslog(LOG_WARNING, "rexmit set addr: %s",
		       error_message(retval));
		goto requeue;

	}
	if ((retval = ZSendPacket(nackpacket->na_packet,
				  nackpacket->na_packsz)) != ZERR_NONE)
		syslog(LOG_WARNING, "rexmit xmit: %s", error_message(retval));

requeue:
	/* reset the timer */
	nackpacket->na_timer = timer_set_rel(rexmit_secs,
					     rexmit,
					     (caddr_t) nackpacket);

#ifdef DEBUG
		if (zdebug)
			dump_nack();
#endif DEBUG
	return;

}

/*
 * Send an acknowledgement to the sending client, by sending back the
 * header from the original notice with the z_kind field changed to either
 * SERVACK or SERVNAK, and the contents of the message either SENT or
 * NOT_SENT, depending on the value of the sent argument.
 */

void
clt_ack(notice, who, sent)
ZNotice_t *notice;
struct sockaddr_in *who;
ZSentType sent;
{
	ZNotice_t acknotice;
	ZPacket_t ackpack;
	int packlen;
	Code_t retval;

	zdbug((LOG_DEBUG,"clt_ack type %d for %d to %s/%d",
	       (int) sent,
	       ntohs(notice->z_port),
	       inet_ntoa(who->sin_addr),
	       ntohs(who->sin_port)));

	if (hostm_find_server(&who->sin_addr) != me_server) {
		zdbug((LOG_DEBUG,"not me"));
		return;
	}
	acknotice = *notice;

	acknotice.z_kind = SERVACK;
	switch (sent) {
	case SENT:
		acknotice.z_message = ZSRVACK_SENT;
		break;
	case NOT_FOUND:
		acknotice.z_message = ZSRVACK_FAIL;
		acknotice.z_kind = SERVNAK;
		break;
	case AUTH_FAILED:
		acknotice.z_kind = SERVNAK;
		/* fall thru */
	case NOT_SENT:
		acknotice.z_message = ZSRVACK_NOTSENT;
		break;
	}

	/* leave room for the trailing null */
	acknotice.z_message_len = strlen(acknotice.z_message) + 1;

	packlen = sizeof(ackpack);

	if ((retval = ZFormatRawNotice(&acknotice,
				       ackpack,
				       packlen,
				       &packlen)) != ZERR_NONE) {
		syslog(LOG_ERR, "clt_ack format: %s",error_message(retval));
		return;
	}
	if ((retval = ZSetDestAddr(who)) != ZERR_NONE) {
		syslog(LOG_WARNING, "clt_ack set addr: %s",
		       error_message(retval));
		return;
	}
	if ((retval = ZSendPacket(ackpack, packlen)) != ZERR_NONE) {
		syslog(LOG_WARNING, "clt_ack xmit: %s", error_message(retval));
		return;
	}
	return;
}

/*
 * An ack has arrived.
 * remove the packet matching this notice from the not-yet-acked queue
 */

static void
nack_cancel(notice, who)
register ZNotice_t *notice;
struct sockaddr_in *who;
{
	register ZNotAcked_t *nacked;
	ZClient_t *client;

	/* set the origin of the ack to the client who is responding,
	 since client_which_client matches on the z_port field */

	notice->z_port = who->sin_port;
	if (!(client = client_which_client(who, notice))) {
		zdbug((LOG_DEBUG,"nack clt not found"));
		return;
	}

	zdbug((LOG_DEBUG,"nack_can: 0x%x %d %d",
	       notice->z_uid.zuid_addr.s_addr,
	       notice->z_uid.tv.tv_sec,
	       notice->z_uid.tv.tv_usec));

	/* search the not-yet-acked list for this packet, and
	   flush it. */
	for (nacked = nacklist->q_forw;
	     nacked != nacklist;
	     nacked = nacked->q_forw)
		if ((nacked->na_client == client))
			if (ZCompareUID((caddr_t) &nacked->na_uid,
				  (caddr_t) &notice->z_uid)) {
				zdbug((LOG_DEBUG,"nack_canceled"));
				timer_reset(nacked->na_timer);
				xfree(nacked->na_packet);
				xremque(nacked);
				return;
			} else {
				zdbug((LOG_DEBUG,
				       "nack_can: not this one 0x%x %d %d",
				       nacked->na_uid.zuid_addr.s_addr,
				       nacked->na_uid.tv.tv_sec,
				       nacked->na_uid.tv.tv_usec));
			}
	zdbug((LOG_DEBUG,"nack not found"));
	return;
}
