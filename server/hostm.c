/* This file is part of the Project Athena Zephyr Notification System.
 * It contains functions for communicating with the HostManager.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /srv/kcr/locker/zephyr/server/Attic/hostm.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987,1988,1991 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#ifndef lint
#ifndef SABER
static char rcsid_hostm_c[] = "$Id: hostm.c,v 1.44 1995-05-31 17:14:34 ghudson Exp $";
#endif
#endif

#include "zserver.h"
#include <sys/socket.h>			/* for AF_INET */

/*
 *
 * External functions:
 *
 * void hostm_dispatch(notice, auth, who, server)
 *	ZNotice_t *notice;
 *	int auth;
 *	struct sockaddr_in *who;
 *	ZServerDesc_t *server;
 *
 * void hostm_flush(host, server)
 *	ZHostList_t *host;
 *	ZServerDesc_t *server;
 *
 * void hostm_transfer(host, server)
 *	ZHostList_t *host;
 *	ZServerDesc_t *server;
 *
 * ZHostList_t *hostm_find_host(addr)
 *	struct in_addr *addr;
 *
 * ZServerDesc_t *hostm_find_server(addr)
 *	struct in_addr *addr;
 *
 * void hostm_shutdown()
 *
 * void hostm_deathgram(sin, server)
 *	struct sockaddr_in *sin;
 * 	ZServerDesc_t *server;
 *
 * void hostm_dump_hosts(fp)
 *	FILE *fp;
 */

/*
 * This module maintains two important structures.
 * all_hosts is an array of all currently known hosts, and which server
 * is responsible for that host.  This list is kept sorted by IP address
 * so that lookups can be fast (binary search).  num_hosts contains the
 * number of hosts to be found in the array.
 *
 */

struct hostlist {
	ZHostList_t *host;		/* ptr to host struct */
	int server_index;		/* index of server in the table */
};

#define	NULLHLT		((struct hostlist *) 0)

static struct hostlist *all_hosts;

static int num_hosts = 0;		/* number of hosts in all_hosts */
static long lose_timo = LOSE_TIMO;

#ifdef __STDC__
# define        P(s) s
#else
# define P(s) ()
#endif

static void host_detach P((register ZHostList_t *host, ZServerDesc_t *server)),
    insert_host P((ZHostList_t *host, ZServerDesc_t *server)),
    remove_host P((ZHostList_t *host));
static Code_t host_attach P((struct sockaddr_in *who, ZServerDesc_t *server));

#undef P

/*
 * We received a HostManager packet.  process accordingly.
 */

/*ARGSUSED*/
Code_t
hostm_dispatch(notice, auth, who, server)
     ZNotice_t *notice;
     int auth;
     struct sockaddr_in *who;
     ZServerDesc_t *server;
{
	ZServerDesc_t *owner;
	ZHostList_t *host = NULLZHLT;
	char *opcode = notice->z_opcode;
	Code_t retval;

#if 0
	zdbug((LOG_DEBUG,"hm_disp"));
#endif

	host = hostm_find_host(&who->sin_addr);
	if (host && host->zh_locked)
		return(ZSRV_REQUEUE);

	if (notice->z_kind == HMACK) {
		/* No longer used. */
		return(ZERR_NONE);
	} else if (notice->z_kind != HMCTL) {
#if 0
		zdbug((LOG_DEBUG, "bogus HM packet"));
#endif
		clt_ack(notice, who, AUTH_FAILED);
		return(ZERR_NONE);
	}
	owner = hostm_find_server(&who->sin_addr);
	if (!strcmp(opcode, HM_ATTACH)) {
#if 0
		zdbug((LOG_DEBUG,"attach %s",inet_ntoa(who)));
#endif
		if (owner == server) {
#if 0
			zdbug((LOG_DEBUG,"no change"));
#endif
			/* Same server owns him.  do nothing */
		} else if (owner) {
			/* He has switched servers.
			   he was lost but has asked server to work for him.
			   We need to transfer him to server */
#if 0
			zdbug((LOG_DEBUG,"hm_disp transfer"));
#endif
			hostm_transfer(host, server);
		} else {
			/* no owner.  attach him to server. */
			if ((retval = host_attach(who, server))
			    != ZERR_NONE) {
				syslog(LOG_WARNING, "hattach failed: %s",
				       error_message(retval));
				return(retval);
			}

		}
		if (server == me_server) {
			server_forward(notice, auth, who);
			ack(notice, who);
		}
	} else if (!strcmp(opcode, HM_BOOT)) {
#if 0
		zdbug((LOG_DEBUG, "boot %s (server %s)",
		       inet_ntoa(who->sin_addr),
		       server->addr));
#endif
		/* Booting is just like flushing and attaching */
		if (owner)		/* if owned, flush */
			hostm_flush(host, owner);
		if ((retval = host_attach(who, server)) != ZERR_NONE) {
			syslog(LOG_WARNING, "hattach failed: %s",
			       error_message(retval));
			return(retval);
		}
		if (server == me_server) {
			server_forward(notice, auth, who);
			ack(notice, who);
		}
	} else if (!strcmp(opcode, HM_FLUSH)) {
#if 0
	        zdbug((LOG_DEBUG, "hm_flush %s (server %s)",
		       inet_ntoa(who->sin_addr),
		       server->addr));
#endif
		if (!owner)
			return(ZERR_NONE);
		/* flush him */
		hostm_flush(host, owner);
		if (server == me_server)
			server_forward(notice, auth, who);
	} else if (!strcmp(opcode, HM_DETACH)) {
#if 0
		zdbug((LOG_DEBUG, "hm_detach %s",inet_ntoa(who_sin_addr)));
#endif
		/* ignore it */
	} else {
		syslog(LOG_WARNING, "hm_disp: unknown opcode %s",opcode);
		return(ZERR_NONE);
	}
	return(ZERR_NONE);
}

/*
 * Flush all information about this host.  Deregister all its clients,
 * flush any user locations, and remove the host from its server.
 * The caller is responsible for informing other servers of this flush
 * (if appropriate).
 */

void
hostm_flush(host, server)
     ZHostList_t *host;
     ZServerDesc_t *server;
{
	register ZClientList_t *clist = NULLZCLT, *clt;

	START_CRITICAL_CODE;

	if (!host) {
	    syslog(LOG_WARNING, "null host flush");
	    return;
	}

#if 0
	zdbug ((LOG_DEBUG,"hostm_flush %s", inet_ntoa (host->zh_addr.sin_addr)));
#endif

	if ((clist = host->zh_clients) != NULLZCLT) {
	  for (clt = clist->q_forw; clt != clist; clt = clist->q_forw) {
	    /* client_deregister frees this client & subscriptions
	       & locations and remque()s the client */
#if 0
	    if (zdebug)
	      syslog (LOG_DEBUG, "hostm_flush clt_dereg %s/%d",
		      inet_ntoa(host->zh_addr.sin_addr),
		      ntohs (clt->zclt_client->zct_sin.sin_port));
#endif
	    client_deregister(clt->zclt_client, host, 1);
	  }
	}

	uloc_hflush(&host->zh_addr.sin_addr);
	host_detach(host, server);

	END_CRITICAL_CODE;

	return;
}

/*
 * send a shutdown to each of our hosts
 */

void
hostm_shutdown()
{
    register ZHostList_t *hosts = otherservers[me_server_idx].zs_hosts;
    register ZHostList_t *host;
    int newserver, i;

#if 0
    zdbug((LOG_DEBUG,"hostm_shutdown"));
#endif
    if (!hosts)
	return;

    for (i = 0; i < nservers; i++){
	if (i == me_server_idx) continue;
	if (otherservers[i].zs_state == SERV_UP)
	    break;
    }
    if (i == nservers)				/* no other servers are up */
	newserver = 0;
    else
	newserver = 1;

    /* kill them all */
    for (host = hosts->q_forw;
	 host != hosts;
	 host = host->q_forw) {
	/* recommend a random, known up server */
	if (newserver) {
	    do
		newserver = (int) (random() % (nservers - 1)) + 1;
	    while (newserver == limbo_server_idx() ||
		   (otherservers[newserver].zs_state != SERV_UP &&
		    otherservers[newserver].zs_state != SERV_TARDY) ||
		   newserver == me_server_idx);
	    hostm_deathgram(&host->zh_addr, &otherservers[newserver]);
	} else
	    hostm_deathgram(&host->zh_addr, NULLZSDT);
    }
    return;
}


/*
 * transfer this host to server's ownership.  The caller must update the
 * other servers.
 */

void
hostm_transfer(host, server)
     ZHostList_t *host;
     ZServerDesc_t *server;
{
	/* we need to unlink and relink him, and change the table entry */
#if 1
	if (zdebug)
	    syslog (LOG_DEBUG, "hostm_transfer %s to %s",
		    inet_ntoa (host->zh_addr.sin_addr), server->addr);
#endif

	/* is this the same server? */
	if (hostm_find_server(&host->zh_addr.sin_addr) == server)
		return;

	START_CRITICAL_CODE;
	
	/* remove from old server's queue */
	xremque(host);

	/* switch servers in the table */
	remove_host(host);
	insert_host(host, server);

	/* insert in our queue */
	xinsque(host, server->zs_hosts);

	END_CRITICAL_CODE;

	return;
}


/*
 * attach the host with return address in who to the server.
 */

static Code_t
host_attach(who, server)
     struct sockaddr_in *who;
     ZServerDesc_t *server;
{
	register ZHostList_t *hlist;
	register ZClientList_t *clist;

	START_CRITICAL_CODE;

#if 0
	if (zdebug)
	    syslog (LOG_DEBUG, "host_attach %s to %s",
		    inet_ntoa (who->sin_addr), server->addr);
#endif
	/* allocate a header */
	if (!(hlist = (ZHostList_t *) xmalloc(sizeof(ZHostList_t)))) {
		syslog(LOG_WARNING, "hm_attach alloc");
		END_CRITICAL_CODE;
		return(ENOMEM);
	}
	/* set up */
	if (!(clist = (ZClientList_t *)xmalloc(sizeof(ZClientList_t)))) {
	        xfree(hlist);
		END_CRITICAL_CODE;
		return(ENOMEM);
	}
	clist->zclt_client = NULLZCNT;
	clist->q_forw = clist->q_back = clist;

	hlist->zh_clients = clist;
	hlist->zh_addr = *who;
	hlist->q_forw = hlist->q_back = hlist;
	hlist->zh_locked = 0;

	/* add to table */
	insert_host(hlist, server);

	/* chain in to the end of the list */
	xinsque(hlist, server->zs_hosts->q_back);

	END_CRITICAL_CODE;

	return(ZERR_NONE);
}

/*
 * detach the host at addr from the server
 * Warning: this routine assumes all the clients have already been removed
 * from this host.
 */

static void
host_detach(host, server)
     register ZHostList_t *host;
     ZServerDesc_t *server;
{
	ZServerDesc_t *server2;

	START_CRITICAL_CODE;

	/* undo what we did in host_attach */
	server2 = hostm_find_server (&host->zh_addr.sin_addr);

	if (server2 != server) {
		syslog(LOG_WARNING,
		       "host_detach: wrong server: %s from %s, found %s",
		       inet_ntoa (host->zh_addr.sin_addr),
		       server->addr,
		       server2->addr);
		END_CRITICAL_CODE;
		return;
	}


	/* all the clients have already been freed */
	xfree(host->zh_clients);

	/* unchain */
	xremque(host);

	/* remove from table */
	remove_host(host);

	xfree(host);

	END_CRITICAL_CODE;
	
	return;
}

/*
 * Build hostmanager recipient name.
 */
static char *
hm_recipient ()
{
    static char *recipient;
    char *realm;

    if (recipient)
	return recipient;

    realm = ZGetRealm ();
    if (!realm)
	realm = "???";
    recipient = (char *) xmalloc (strlen (realm) + 4);
    strcpy (recipient, "hm@");
    strcat (recipient, realm);
    return recipient;
}

/*
 * Send a shutdown message to the HostManager at sin, recommending him to
 * use server
 */

void
hostm_deathgram(sin, server)
     struct sockaddr_in *sin;
     ZServerDesc_t *server;
{
	Code_t retval;
	int shutlen;
	ZNotice_t shutnotice;
	char *shutpack;

#if 0
	zdbug((LOG_DEBUG,"deathgram %s",inet_ntoa(*sin)));
#endif

	/* fill in the shutdown notice */

	shutnotice.z_kind = HMCTL;
	shutnotice.z_port = sock_sin.sin_port; /* we are sending it */
	shutnotice.z_class = HM_CTL_CLASS;
	shutnotice.z_class_inst = HM_CTL_SERVER;
	shutnotice.z_opcode = SERVER_SHUTDOWN;
	shutnotice.z_sender = HM_CTL_SERVER;
	shutnotice.z_recipient = hm_recipient ();
	shutnotice.z_default_format = "";
	shutnotice.z_num_other_fields = 0;

	if (server) {
		shutnotice.z_message = server->addr;
		shutnotice.z_message_len = strlen(shutnotice.z_message) + 1;
#if 0
		zdbug((LOG_DEBUG, "suggesting %s",shutnotice.z_message));
#endif
	} else {
		shutnotice.z_message = NULL;
		shutnotice.z_message_len = 0;
	}

	if ((retval = ZFormatNotice(&shutnotice,
				    &shutpack,
				    &shutlen,
				    ZNOAUTH)) != ZERR_NONE) {
		syslog(LOG_ERR, "hm_shut format: %s",error_message(retval));
		return;
	}
	if ((retval = ZSetDestAddr(sin)) != ZERR_NONE) {
		syslog(LOG_WARNING, "hm_shut set addr: %s",
		       error_message(retval));
		xfree(shutpack);	/* free allocated storage */
		return;
	}
	/* don't wait for ack! */
	if ((retval = ZSendPacket(shutpack, shutlen, 0)) != ZERR_NONE) {
		syslog(LOG_WARNING, "hm_shut xmit: %s", error_message(retval));
		xfree(shutpack);	/* free allocated storage */
		return;
	}
	xfree(shutpack);		/* free allocated storage */
	return;
}

/*
 * Routines for maintaining the host array.
 */

/*
 * Binary search on the host table to find this host.
 */

ZHostList_t *
hostm_find_host(addr)
     struct in_addr *addr;
{
	register int i, rlo, rhi;

	if (!all_hosts)
		return(NULLZHLT);

	/* i is the current host we are checking */
	/* rlo is the lowest we will still check, rhi is the highest we will
	   still check */

	i = num_hosts >> 1;		/* start in the middle */
	rlo = 0;
	rhi = num_hosts - 1;		/* first index is 0 */

	while ((all_hosts[i].host)->zh_addr.sin_addr.s_addr != addr->s_addr) {
		if ((all_hosts[i].host)->zh_addr.sin_addr.s_addr < addr->s_addr)
			rlo = i + 1;
		else
			rhi = i - 1;
		if (rhi - rlo < 0)
			return(NULLZHLT);
		i = (rhi + rlo) >> 1; /* split the diff */
	}
	return(all_hosts[i].host);
}

/*
 * Binary search on the host table to find this host's server.
 */

ZServerDesc_t *
hostm_find_server(addr)
     struct in_addr *addr;
{
	register int i, rlo, rhi;

	if (!all_hosts)
		return(NULLZSDT);

	/* i is the current host we are checking */
	/* rlo is the lowest we will still check, rhi is the highest we will
	   still check */

	i = num_hosts >> 1;		/* start in the middle */
	rlo = 0;
	rhi = num_hosts - 1;		/* first index is 0 */

	while ((all_hosts[i].host)->zh_addr.sin_addr.s_addr != addr->s_addr) {
		if ((all_hosts[i].host)->zh_addr.sin_addr.s_addr < addr->s_addr)
			rlo = i + 1;
		else
			rhi = i - 1;
		if (rhi - rlo < 0)
			return(NULLZSDT);
		i = (rhi + rlo) >> 1; /* split the diff */
	}
	return(&otherservers[all_hosts[i].server_index]);
}

/*
 * Insert the host and server into the sorted array of hosts.
 */

static void
insert_host(host, server)
     ZHostList_t *host;
     ZServerDesc_t *server;
{
	struct hostlist *oldlist;
	register int i = 0;

#if 0
	zdbug ((LOG_DEBUG,"insert_host %s %s",
		inet_ntoa(host->zh_addr.sin_addr), server->addr));
#endif
	if (hostm_find_host(&host->zh_addr.sin_addr))
		return;

	START_CRITICAL_CODE;

	num_hosts++;
	oldlist = all_hosts;

	if (!(all_hosts = (struct hostlist *) xmalloc(num_hosts * sizeof(struct hostlist)))) {
		syslog(LOG_CRIT, "insert_host: nomem");
		abort();
	}

	if (!oldlist) {			/* this is the first */
		all_hosts[0].host = host;
		all_hosts[0].server_index = server - otherservers;
		END_CRITICAL_CODE;
		return;
	}

	/* copy old pointers */
	while ((i < (num_hosts - 1)) &&
	       ((oldlist[i].host)->zh_addr.sin_addr.s_addr < host->zh_addr.sin_addr.s_addr)) {
		all_hosts[i] = oldlist[i];
		i++;
	}
	/* add this one */
	all_hosts[i].host = host;
	all_hosts[i++].server_index = server - otherservers;

	/* copy the rest */
	while (i < num_hosts) {
		all_hosts[i] = oldlist[i - 1];
		i++;
	}
	xfree(oldlist);

	END_CRITICAL_CODE;

#if defined (DEBUG) && 0
        if (zdebug) {
                register int i = 0;
		for (i = 0; i < num_hosts; i++)
		    syslog(LOG_DEBUG, "%d: %s %s",i,
			   inet_ntoa ((all_hosts[i].host)->zh_addr.sin_addr),
			   otherservers[all_hosts[i].server_index]->addr);
        }
#endif
	return;
}

/*
 * remove the host from the array of known hosts.
 */

static void
remove_host(host)
     ZHostList_t *host;
{
	struct hostlist *oldlist;
	register int i = 0;

#if 0
	zdbug((LOG_DEBUG,"remove_host %s", inet_ntoa(host->zh_addr.sin_addr)));
#endif
	if (!hostm_find_host(&host->zh_addr.sin_addr))
		return;

	START_CRITICAL_CODE;
	
	if (--num_hosts == 0) {
#if 0
		zdbug((LOG_DEBUG,"last host"));
#endif
		xfree (all_hosts);
		all_hosts = NULLHLT;
		END_CRITICAL_CODE;
		return;
	}

	oldlist = all_hosts;

	if (!(all_hosts = (struct hostlist *) xmalloc(num_hosts * sizeof(struct hostlist)))) {
		syslog(LOG_CRIT, "remove_host: nomem");
		abort();
	}

	/* copy old pointers */
	while (i < num_hosts && (oldlist[i].host)->zh_addr.sin_addr.s_addr < host->zh_addr.sin_addr.s_addr) {
		all_hosts[i] = oldlist[i];
		i++;
	}

	i++;				/* skip over this one */

	/* copy the rest */
	while (i <= num_hosts) {
		all_hosts[i - 1] = oldlist[i];
		i++;
	}
	xfree (oldlist);

	END_CRITICAL_CODE;

	return;
}

/*
 * Assumes that SIGFPE is blocked when called; this is true if called from a
 * signal handler
 */

void
hostm_dump_hosts(fp)
     FILE *fp;
{
	register int i;
	for (i = 0; i < num_hosts; i++) {
		(void) fprintf(fp, "%s/%d:\n", 
			       inet_ntoa((all_hosts[i].host)->zh_addr.sin_addr),
			       all_hosts[i].server_index);
		client_dump_clients(fp,(all_hosts[i].host)->zh_clients);
	}
	return;
}

/*
 * Readjust server-array indices according to the supplied new vector.
 */

void
hostm_renumber_servers (srv)
     int *srv;
{
    int i;
    for (i = 0; i < num_hosts; i++) {
	int idx = srv[all_hosts[i].server_index];
	if (idx < 0) {
	    syslog (LOG_ERR, "hostm_renumber_servers error: [%d] = %d",
		    all_hosts[i].server_index, idx);
	    idx = 0;
	}
	all_hosts[i].server_index = idx;
    }
}
