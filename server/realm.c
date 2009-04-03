#include "zserver.h"
#include <sys/socket.h>

Unacked *rlm_nacklist = NULL;   /* not acked list for realm-realm
                                   packets */
ZRealm *otherrealms;             /* points to an array of the known
                                   servers */
int nrealms = 0;                /* number of other realms */

/*
 * External Routines:
 *
 * ZRealm *realm_which_realm(struct sockaddr_in *who)
 * figures out if this packet came from another realm's server
 *
 * ZRealm *realm_get_realm_by_pid(int pid)
 * figures out which realm a child handler was for
 *
 * void kill_realm_pids()
 * kills all ticket getting childen
 *
 * char *realm_expand_realm(char *realmname)
 * figures out what an abbreviated realm expands to
 *
 * Code_t realm_send_realms()
 * loops through all realms for a brain dump
 *
 * int realm_bound_for_realm(char *realm, char *recip)
 * figures out if recip is in realm, expanding recip's realm
 *
 * int realm_sender_in_realm(char *realm, char *sender)
 * figures out if sender is in realm
 * 
 * ZRealm *realm_get_realm_by_name(char *name)
 * finds a realm struct from the realm array by name, tries expansion
 *
 * Code_t realm_dispatch(ZNotice_t *notice, int auth, struct sockaddr_in *who,
 *                       Server *server)
 * dispatches a message from a foreign realm
 *
 * void realm_init()
 * sets up the realm module
 * 
 * void realm_deathgram()
 * tells other realms this server is going down
 * 
 * void realm_wakeup()
 * tells other realms to resend their idea of their subs to us
 *
 * Code_t realm_control_dispatch(ZNotice_t *notice, int auth,
 *                               struct sockaddr_in *who, Server *server,
 *				 ZRealm *realm)
 * dispatches a foreign realm control message
 *
 * void realm_handoff(ZNotice_t *notice, int auth, struct sockaddr_in *who,
 *                    ZRealm *realm, int ack_to_sender)
 * hands off a message to another realm
 *
 * void realm_dump_realms(File *fp)
 * do a database dump of foreign realm info
 *
 */
static void realm_sendit(ZNotice_t *notice, struct sockaddr_in *who, int auth, ZRealm *realm, int ack_to_sender);
#ifdef HAVE_KRB5
static Code_t realm_sendit_auth(ZNotice_t *notice, struct sockaddr_in *who, int auth, ZRealm *realm, int ack_to_sender);
#endif
static void rlm_ack(ZNotice_t *notice, Unacked *nacked);
static void rlm_nack_cancel(ZNotice_t *notice, struct sockaddr_in *who);
static void rlm_rexmit(void *arg);
static Code_t realm_ulocate_dispatch(ZNotice_t *notice,int auth,struct sockaddr_in *who,Server *server,ZRealm *realm);
static Code_t realm_new_server(struct sockaddr_in *, ZNotice_t *, ZRealm *);
static Code_t realm_set_server(struct sockaddr_in *, ZRealm *);
#ifdef HAVE_KRB5
static Code_t ticket_retrieve(ZRealm *realm);
static int ticket_lookup(char *realm);
#endif

static int
realm_get_idx_by_addr(ZRealm *realm,
		      struct sockaddr_in *who)
{
    struct sockaddr_in *addr;
    int b;

    /* loop through the realms */
    for (addr = realm->addrs, b = 0; b < realm->count; b++, addr++)
	if (addr->sin_addr.s_addr == who->sin_addr.s_addr)
	    return(b);
    
    return 0;
}

const char *
realm_expand_realm(char *realmname)
{
    ZRealm *realm;
    int a;

    /* First, look for an exact match (case insensitive) */
#if defined(HAVE_KRB4) || defined(HAVE_KRB5)
    if (!strcasecmp(ZGetRealm(), realmname))
	return(ZGetRealm());
#endif

    for (realm = otherrealms, a = 0; a < nrealms; a++, realm++)
      if (!strcasecmp(realm->name, realmname))
	return(realm->name);

    /* No exact match. See if there's a partial match */
#if defined(HAVE_KRB4) || defined(HAVE_KRB5)
    if (!strncasecmp(ZGetRealm(), realmname, strlen(realmname)))
	return(ZGetRealm());
#endif

    for (realm = otherrealms, a = 0; a < nrealms; a++, realm++)
	if (!strncasecmp(realm->name, realmname, strlen(realmname)))
	    return(realm->name);
    return(realmname);
}

ZRealm *
realm_get_realm_by_pid(int pid)
{
    ZRealm *realm;
    int a;

    for (realm = otherrealms, a = 0; a < nrealms; a++, realm++)
	if (realm->child_pid == pid)
	    return(realm);
   
    return 0;
}

void
kill_realm_pids(void)
{
    ZRealm *realm;
    int a;

    for (realm = otherrealms, a = 0; a < nrealms; a++, realm++)
	if (realm->child_pid != 0)
	    kill(realm->child_pid, 9);
   
    return;
}

ZRealmname *
get_realm_lists(char *file)
{
    ZRealmname *rlm_list, *rlm;
    int ii, nused, ntotal;
    FILE *fp;
    char buf[REALM_SZ + MAXHOSTNAMELEN + 1]; /* one for newline */
    char realm[REALM_SZ], server[MAXHOSTNAMELEN + 1];
  
    nused = 0;
    if (!(fp = fopen(file, "r")))
	return((ZRealmname *)0);
  
    /* start with 16, realloc if necessary */
    ntotal = 16;
    rlm_list = (ZRealmname *)malloc(ntotal * sizeof(ZRealmname));
    if (!rlm_list) {
	syslog(LOG_CRIT, "get_realm_lists malloc");
	abort();
    }

    while (fgets(buf, REALM_SZ + MAXHOSTNAMELEN + 1, fp)) {
	if (sscanf(buf, "%s %s", realm, server) != 2) {
	    syslog(LOG_CRIT, "bad format in %s", file);
	    abort();
	}
	for (ii = 0; ii < nused; ii++) {
	    /* look for this realm */
	    if (!strcmp(rlm_list[ii].name, realm))
		break;
	}
	if (ii < nused) {
	    rlm = &rlm_list[ii];
	    if (rlm->nused +1 >= rlm->nservers) {
		/* make more space */
		rlm->servers = (char **)realloc((char *)rlm->servers, 
						(unsigned)rlm->nservers * 2 * 
						sizeof(char *));
		if (!rlm->servers) {
		    syslog(LOG_CRIT, "get_realm_lists realloc");
		    abort();
		}
		rlm->nservers *= 2;
	    }
	    rlm->servers[rlm->nused++] = strsave(server);
	} else {
	    /* new realm */
	    if (nused + 1 >= ntotal) {
		/* make more space */
		rlm_list = (ZRealmname *)realloc((char *)rlm_list,
						(unsigned)ntotal * 2 * 
						sizeof(ZRealmname));
		if (!rlm_list) {
		    syslog(LOG_CRIT, "get_realm_lists realloc");
		    abort();
		}
		ntotal *= 2;
	    }
	    rlm = &rlm_list[nused++];
	    strcpy(rlm->name, realm);
	    rlm->nused = 0;
	    rlm->nservers = 16;
	    rlm->servers = (char **)malloc(rlm->nservers * sizeof(char *));
	    if (!rlm->servers) {
		syslog(LOG_CRIT, "get_realm_lists malloc");
		abort();
	    }
	    rlm->servers[rlm->nused++] = strsave(server);
	}
    }
    if (nused + 1 >= ntotal) {
	rlm_list = (ZRealmname *)realloc((char *)rlm_list,
					(unsigned)(ntotal + 1) * 
					sizeof(ZRealmname));
	if (!rlm_list) {
	    syslog(LOG_CRIT, "get_realm_lists realloc");
	    abort();
	}
    }
    *rlm_list[nused].name = '\0';
  
    fclose(fp);
    return(rlm_list);
}

Code_t 
realm_send_realms(void)
{
    int cnt, retval;
    for (cnt = 0; cnt < nrealms; cnt++) {
	retval = subscr_send_realm_subs(&otherrealms[cnt]);
	if (retval != ZERR_NONE)
	    return(retval);
    }
    return ZERR_NONE;
}

int
realm_bound_for_realm(const char *realm, char *recip)
{
    char *rlm = NULL;
    int remote = strcmp(ZGetRealm(), realm);
    
    if (recip)
      rlm = strchr(recip, '@');
    
    if (!rlm && !remote) 
	return 1;

    if (rlm && strcmp(realm_expand_realm(rlm + 1), realm) == 0)
	return 1;

    return 0;
}

int
realm_sender_in_realm(const char *realm, char *sender)
{
    char *rlm = NULL;
    int remote = strcmp(ZGetRealm(), realm);

    if (sender)
	rlm = strchr(sender, '@');

    if (!rlm && !remote)
	return 1;

    if (rlm && strcmp((rlm + 1), realm) == 0)
	return 1;

    return 0;
}

int sender_in_realm(ZNotice_t *notice)
{
  char *realm;

  realm = strchr(notice->z_sender, '@');

  if (!realm || !strcmp(realm + 1, ZGetRealm()))
    return 1;

  return 0;
}

ZRealm *
realm_which_realm(struct sockaddr_in *who)
{
    ZRealm *realm;
    struct sockaddr_in *addr;
    int a, b;

    if (who->sin_port != srv_addr.sin_port)
	return 0;

    /* loop through the realms */
    for (realm = otherrealms, a = 0; a < nrealms; a++, realm++)
	/* loop through the addresses for the realm */
	for (addr = realm->addrs, b = 0; b < realm->count; b++, addr++)
	    if (addr->sin_addr.s_addr == who->sin_addr.s_addr)
		return(realm);
  
    return 0;
}

ZRealm *
realm_get_realm_by_name(char *name)
{
    int a;
    ZRealm *realm;

    /* First, look for an exact match (case insensitive) */
    for (realm = otherrealms, a = 0; a < nrealms; a++, realm++)
	if (!strcasecmp(realm->name, name))
	    return(realm);

    /* Failing that, look for an inexact match */
    for (realm = otherrealms, a = 0; a < nrealms; a++, realm++)
	if (!strncasecmp(realm->name, name, strlen(name)))
	    return(realm);

    return 0;
}

static void
rlm_nack_cancel(register ZNotice_t *notice,
		struct sockaddr_in *who)
{
    register ZRealm *which = realm_which_realm(who);
    register Unacked *nacked;
  
    zdbug((LOG_DEBUG, "rlm_nack_cancel: %s:%08X,%08X",
           inet_ntoa(notice->z_uid.zuid_addr),
           notice->z_uid.tv.tv_sec, notice->z_uid.tv.tv_usec));

    if (!which) {
	syslog(LOG_ERR, "non-realm ack?");
	return;
    }

    for (nacked = rlm_nacklist; nacked; nacked = nacked->next) {
	if (&otherrealms[nacked->dest.rlm.rlm_idx] == which) {
	    /* First, note the realm appears to be up */
	    which->state = REALM_UP;
	    if (ZCompareUID(&nacked->uid, &notice->z_uid)) {
		timer_reset(nacked->timer);
        
		if (nacked->ack_addr.sin_addr.s_addr)
		    rlm_ack(notice, nacked);
        
		/* free the data */
		free(nacked->packet);
		Unacked_delete(nacked);
		free(nacked);
		return;
	    }
	}
    }
    return;
}

static void
rlm_ack(ZNotice_t *notice,
	Unacked *nacked)
{
    ZNotice_t acknotice;
    ZPacket_t ackpack;
    int packlen;
    Code_t retval;
  
    /* tell the original sender the result */
    acknotice = *notice;
    acknotice.z_message_len = strlen(acknotice.z_message) + 1;
  
    packlen = sizeof(ackpack);
  
    if ((retval = ZFormatSmallRawNotice(&acknotice, ackpack, &packlen)) 
	!= ZERR_NONE) {
	syslog(LOG_ERR, "rlm_ack format: %s",
	       error_message(retval));
	return;
    }
    zdbug((LOG_DEBUG, "rlm_ack sending to %s/%d",
	   inet_ntoa(nacked->ack_addr.sin_addr),
	   ntohs(nacked->ack_addr.sin_port)));
    if ((retval = ZSetDestAddr(&nacked->ack_addr)) != ZERR_NONE) {
	syslog(LOG_WARNING, "rlm_ack set addr: %s",
	       error_message(retval));
	return;
    }
    if ((retval = ZSendPacket(ackpack, packlen, 0)) != ZERR_NONE) {
	syslog(LOG_WARNING, "rlm_ack xmit: %s", error_message(retval));
	return;
    }
}

Code_t
realm_dispatch(ZNotice_t *notice,
	       int auth,
	       struct sockaddr_in *who,
	       Server *server)
{
    ZRealm *realm;
    struct sockaddr_in newwho;
    Code_t status = ZERR_NONE;
    char rlm_recipient[REALM_SZ + 1];
    int external = 0;
    String *notice_class;

    if (notice->z_kind == SERVACK || notice->z_kind == SERVNAK) {
	rlm_nack_cancel(notice, who);
	return(ZERR_NONE);
    }
    /* set up a who for the real origin */
    memset((caddr_t) &newwho, 0, sizeof(newwho));
    newwho.sin_family = AF_INET;
    newwho.sin_addr.s_addr = notice->z_sender_addr.s_addr;
    newwho.sin_port = hm_port;
    
    /* check if it's a control message */
    realm = realm_which_realm(who);

    notice_class = make_string(notice->z_class,1);
    
    if (class_is_admin(notice_class)) {
	syslog(LOG_WARNING, "%s sending admin opcode %s",
	       realm->name, notice->z_opcode);
    } else if (class_is_hm(notice_class)) {
	syslog(LOG_WARNING, "%s sending hm opcode %s",
	       realm->name, notice->z_opcode);
    } else if (class_is_control(notice_class)) {
	status = realm_control_dispatch(notice, auth, who,
					server, realm);
    } else if (class_is_ulogin(notice_class)) {
	/* don't need to forward this */
	if (server == me_server) {
            sprintf(rlm_recipient, "@%s", realm->name);
            notice->z_recipient = rlm_recipient;

            sendit(notice, 1, who, 0);
	}
    } else if (class_is_ulocate(notice_class)) {
	status = realm_ulocate_dispatch(notice, auth, who, server, realm);
    } else {
	/* redo the recipient */
	if (*notice->z_recipient == '\0') {
	    sprintf(rlm_recipient, "@%s", realm->name);
	    notice->z_recipient = rlm_recipient;
	    external = 0;
	} else if (realm_bound_for_realm(ZGetRealm(), notice->z_recipient)
		   && *notice->z_recipient == '@') 
	{
	    /* we're responsible for getting this message out */
	    external = 1;
	    notice->z_recipient = "";
	}
          
	/* otherwise, send to local subscribers */
	sendit(notice, auth, who, external);
    }
        
    return(status);
}

void
realm_init(void)
{
    Client *client;
    ZRealmname *rlmnames;
    ZRealm *rlm;
    int ii, jj, found;
    struct in_addr *addresses;
    struct hostent *hp;
    char list_file[128];
    char rlmprinc[MAX_PRINCIPAL_SIZE];

    sprintf(list_file, "%s/zephyr/%s", SYSCONFDIR, REALM_LIST_FILE);
    rlmnames = get_realm_lists(list_file);
    if (!rlmnames) {
	zdbug((LOG_DEBUG, "No other realms"));
	nrealms = 0;
	return;
    }
    
    for (nrealms = 0; *rlmnames[nrealms].name; nrealms++);
    
    otherrealms = (ZRealm *)malloc(nrealms * sizeof(ZRealm));
    if (!otherrealms) {
	syslog(LOG_CRIT, "malloc failed in realm_init");
	abort();
    }

    for (ii = 0; ii < nrealms; ii++) {
	rlm = &otherrealms[ii];
	strcpy(rlm->name, rlmnames[ii].name);
	
	addresses = (struct in_addr *)malloc(rlmnames[ii].nused * 
					     sizeof(struct in_addr));
	if (!addresses) {
	    syslog(LOG_CRIT, "malloc failed in realm_init");
	    abort();
	}
	/* convert names to addresses */
	found = 0;
	for (jj = 0; jj < rlmnames[ii].nused; jj++) {
	    hp = gethostbyname(rlmnames[ii].servers[jj]);
	    if (hp) {
		memmove((caddr_t) &addresses[found], (caddr_t)hp->h_addr, 
			sizeof(struct in_addr));
		found++;
	    } else
		syslog(LOG_WARNING, "hostname failed, %s", 
		       rlmnames[ii].servers[jj]);
	    /* free the hostname */
	    free(rlmnames[ii].servers[jj]);
	}
	rlm->count = found;
	rlm->addrs = (struct sockaddr_in *)malloc(found * 
						  sizeof (struct sockaddr_in));
	if (!rlm->addrs) {
	    syslog(LOG_CRIT, "malloc failed in realm_init");
	    abort();
	}
	for (jj = 0; jj < rlm->count; jj++) {
	    rlm->addrs[jj].sin_family = AF_INET;
	    /* use the server port */
	    rlm->addrs[jj].sin_port = srv_addr.sin_port;
	    rlm->addrs[jj].sin_addr = addresses[jj];
	}
	client = (Client *) malloc(sizeof(Client));
	if (!client) {
	    syslog(LOG_CRIT, "malloc failed in realm_init");
	    abort();
	}
	memset(&client->addr, 0, sizeof(struct sockaddr_in));
#ifdef HAVE_KRB5
        client->session_keyblock = NULL;
#else
#ifdef HAVE_KRB4
	memset(&client->session_key, 0, sizeof(client->session_key));
#endif
#endif
	snprintf(rlmprinc, MAX_PRINCIPAL_SIZE, "%s.%s@%s", SERVER_SERVICE, SERVER_INSTANCE, 
		rlm->name);
	client->principal = make_string(rlmprinc, 0);
	client->last_send = 0;
	client->last_ack = NOW;
	client->subs = NULL;
	client->realm = rlm;
	client->addr.sin_family = 0;
	client->addr.sin_port = 0;
	client->addr.sin_addr.s_addr = 0;
    
	rlm->client = client;
	rlm->idx = (rlm->count) ? random() % rlm->count : 0;
	rlm->subs = NULL;
	rlm->remsubs = NULL;
	rlm->child_pid = 0;
	/* Assume the best */
	rlm->state = REALM_TARDY;
	rlm->have_tkt = 1;
	free(rlmnames[ii].servers);
	free(addresses);
    }
    free(rlmnames);
}

void
realm_deathgram(Server *server)
{
    ZRealm *realm;
    int jj = 0;

    /* Get it out once, and assume foreign servers will share */
    for (realm = otherrealms, jj = 0; jj < nrealms; jj++, realm++) {
	ZNotice_t snotice;
	char *pack;
	char rlm_recipient[REALM_SZ + 1];
	int packlen, retval;
    
	memset (&snotice, 0, sizeof (snotice));

	snotice.z_kind = ACKED;
	snotice.z_port = srv_addr.sin_port;
	snotice.z_class = ZEPHYR_CTL_CLASS;
	snotice.z_class_inst = ZEPHYR_CTL_REALM;
	snotice.z_opcode = SERVER_SHUTDOWN;
	snotice.z_sender = myname; /* my host name */
	sprintf(rlm_recipient, "@%s", realm->name);
	snotice.z_recipient = rlm_recipient;
	snotice.z_default_format = "";
	snotice.z_num_other_fields = 0;
	snotice.z_default_format = "";
	snotice.z_message = (server) ? server->addr_str : NULL;
	snotice.z_message_len = (server) ? strlen(server->addr_str) + 1 : 0;

	zdbug((LOG_DEBUG, "rlm_deathgram: suggesting %s to %s", 
	       (server) ? server->addr_str : "nothing", realm->name));

#ifdef HAVE_KRB5
	if (!ticket_lookup(realm->name))
	    if ((retval = ticket_retrieve(realm)) != ZERR_NONE) {
		syslog(LOG_WARNING, "rlm_deathgram failed: %s", 
		       error_message(retval));
		return;
	    }
#endif

	if ((retval = ZFormatNotice(&snotice, &pack, &packlen, ZCAUTH)) 
	    != ZERR_NONE) 
	{
	    syslog(LOG_WARNING, "rlm_deathgram format: %s",
		   error_message(retval));
	    return;
	}
	if ((retval = ZParseNotice(pack, packlen, &snotice)) != ZERR_NONE) {
	    syslog(LOG_WARNING, "rlm_deathgram parse: %s",
		   error_message(retval));
	    free(pack);
	    return;
	}

	realm_handoff(&snotice, 1, NULL, realm, 0);
	free(pack);
    }
}

void
realm_wakeup(void)
{
    int jj, found = 0;
    ZRealm *realm;
    
    for (jj = 1; jj < nservers; jj++) {    /* skip limbo server */
	if (jj != me_server_idx && otherservers[jj].state == SERV_UP)
	    found++;
    }
  
    if (nservers < 2 || !found) {
	/* if we're the only server up, send a REALM_BOOT to one of their 
	   servers here */
	for (realm = otherrealms, jj = 0; jj < nrealms; jj++, realm++) {
	    ZNotice_t snotice;
	    char *pack;
	    char rlm_recipient[REALM_SZ + 1];
	    int packlen, retval;
	    
	    memset (&snotice, 0, sizeof (snotice));

	    snotice.z_opcode = REALM_BOOT;
	    snotice.z_port = srv_addr.sin_port;
	    snotice.z_class_inst = ZEPHYR_CTL_REALM;
	    snotice.z_class = ZEPHYR_CTL_CLASS;
	    snotice.z_recipient = "";
	    snotice.z_kind = ACKED;
	    snotice.z_num_other_fields = 0;
	    snotice.z_default_format = "";
	    snotice.z_sender = myname; /* my host name */
	    sprintf(rlm_recipient, "@%s", realm->name);
	    snotice.z_recipient = rlm_recipient;
	    snotice.z_default_format = "";
	    snotice.z_message = NULL;
	    snotice.z_message_len = 0;

#ifdef HAVE_KRB5
	    if (!ticket_lookup(realm->name))
		if ((retval = ticket_retrieve(realm)) != ZERR_NONE) {
		    syslog(LOG_WARNING, "rlm_wakeup failed: %s", 
			   error_message(retval));
		    continue;
		}
#endif

	    if ((retval = ZFormatNotice(&snotice, &pack, &packlen, ZAUTH)) 
		!= ZERR_NONE) 
	    {
		syslog(LOG_WARNING, "rlm_wakeup format: %s",
		       error_message(retval));
		return;
	    }
	    if ((retval = ZParseNotice(pack, packlen, &snotice)) 
		!= ZERR_NONE) {
		syslog(LOG_WARNING, "rlm_wakeup parse: %s",
		       error_message(retval));
		free(pack);
		return;
	    }

	    realm_handoff(&snotice, 1, NULL, realm, 0);
	    free(pack);
	}      
    }
}

static Code_t
realm_ulocate_dispatch(ZNotice_t *notice,
		       int auth,
		       struct sockaddr_in *who,
		       Server *server,
		       ZRealm *realm)
{
    register char *opcode = notice->z_opcode;
  
    if (!auth) {
	syslog(LOG_WARNING, "unauth locate msg from %s (%s/%s/%s)",
	       inet_ntoa(who->sin_addr), 
	       notice->z_class, notice->z_class_inst, 
	       notice->z_opcode); /* XXX */
	clt_ack(notice, who, AUTH_FAILED);
	return(ZERR_NONE);
    }
    
    if (!strcmp(opcode, REALM_REQ_LOCATE)) {
	ack(notice, who);
	ulogin_realm_locate(notice, who, realm);
    } else if (!strcmp(opcode, REALM_ANS_LOCATE)) {
	ack(notice, who);
	ulogin_relay_locate(notice, who);
    } else {
	syslog(LOG_WARNING, "%s unknown/illegal loc opcode %s",
	       realm->name, opcode);
	nack(notice, who);
    }
    
    return(ZERR_NONE);
}


Code_t
realm_control_dispatch(ZNotice_t *notice,
		       int auth,
		       struct sockaddr_in *who,
		       Server *server,
		       ZRealm *realm)
{
    register char *opcode = notice->z_opcode;
    Code_t status;

    if (!auth) {
	syslog(LOG_WARNING, "unauth ctl msg from %s (%s/%s/%s)",
	       inet_ntoa(who->sin_addr), 
	       notice->z_class, notice->z_class_inst, 
	       notice->z_opcode); /* XXX */
	if (server == me_server)
	    clt_ack(notice, who, AUTH_FAILED);
	return(ZERR_NONE);
    }

    if (strcmp(notice->z_class_inst, ZEPHYR_CTL_REALM)) {
	syslog(LOG_WARNING, "Invalid rlm_dispatch instance %s",
	       notice->z_class_inst);
	return(ZERR_NONE);
    }

    if (!strcmp(opcode, REALM_REQ_SUBSCRIBE) || !strcmp(opcode, REALM_ADD_SUBSCRIBE)) {
	/* try to add subscriptions */
	/* attempts to get defaults are ignored */
	if ((status = subscr_foreign_user(notice, who, server, realm)) != ZERR_NONE) {
	    clt_ack(notice, who, AUTH_FAILED);
	} else if (server == me_server) {
	    server_forward(notice, auth, who);
	    ack(notice, who);
	}
    } else if (!strcmp(opcode, REALM_UNSUBSCRIBE)) {
	/* try to remove subscriptions */
	if ((status = subscr_realm_cancel(who, notice, realm)) != ZERR_NONE) {
	    clt_ack(notice, who, NOT_FOUND);
	} else if (server == me_server) {
	    server_forward(notice, auth, who);
	    ack(notice, who);
	}
    } else if (!strcmp(opcode, REALM_BOOT)) {
	zdbug((LOG_DEBUG, "got a REALM_BOOT from %d (me %d)", server, me_server));
	realm->state = REALM_STARTING;
	realm_set_server(who, realm);
#ifdef REALM_MGMT
	/* resend subscriptions but only if this was to us */
	if (server == me_server) {
	    if ((status = subscr_realm_subs(realm)) != ZERR_NONE) {
		clt_ack(notice, who, NOT_FOUND);
	    } else {
		/* do forward the hint in case it ever matters */
		server_forward(notice, auth, who);
		ack(notice, who);
	    }
	}
#endif
    } else if (!strcmp(opcode, SERVER_SHUTDOWN)) {
	/* try to remove subscriptions */
	if ((status = realm_new_server(who, notice, realm)) != ZERR_NONE) {
	    clt_ack(notice, who, NOT_FOUND);
	} else if (server == me_server) {
	    server_forward(notice, auth, who);
	    ack(notice, who);
	}
    } else {
	syslog(LOG_WARNING, "%s unknown/illegal ctl opcode %s",
	       realm->name, opcode);
	if (server == me_server)
	    nack(notice, who);
	return(ZERR_NONE);
    }
    return(ZERR_NONE);
}

static Code_t
realm_new_server(struct sockaddr_in *sin,
		 ZNotice_t *notice,
		 ZRealm *realm)
{
    unsigned long addr;
    ZRealm *rlm;
    struct sockaddr_in sinaddr;
    int srvidx;

    if (!realm)
	return ZSRV_NORLM;

    srvidx = realm_get_idx_by_addr(realm, sin);
    zdbug((LOG_DEBUG, "rlm_new_srv: message from %d in %s (%s)", 
	   srvidx, realm->name, inet_ntoa(sin->sin_addr)));
    if (realm->idx == srvidx) {
	if (notice->z_message_len) {
	    addr = inet_addr(notice->z_message);
	    sinaddr.sin_addr.s_addr = addr;
	    rlm = realm_which_realm(&sinaddr);
	    /* Not exactly */
	    if (!rlm || (rlm != realm))
		return ZSRV_NORLM;
	    realm->idx = realm_get_idx_by_addr(realm, &sinaddr);
	} else {
	    realm->idx = (realm->idx + 1) % realm->count;
	} 
	zdbug((LOG_DEBUG, "rlm_new_srv: switched servers (%s)", inet_ntoa((realm->addrs[realm->idx]).sin_addr)));
    } else {
      zdbug((LOG_DEBUG, "rlm_new_srv: not switching servers (%s)", inet_ntoa((realm->addrs[realm->idx]).sin_addr)));
    }
    return 0;
}

static Code_t
realm_set_server(struct sockaddr_in *sin,
		 ZRealm *realm)
{
    ZRealm *rlm;

    rlm = realm_which_realm(sin);
    /* Not exactly */
    if (!rlm || (rlm != realm))
	return ZSRV_NORLM;
    realm->idx = realm_get_idx_by_addr(realm, sin);
    zdbug((LOG_DEBUG, "rlm_pick_srv: switched servers (%s)", inet_ntoa((realm->addrs[realm->idx]).sin_addr)));

    return 0;
}

void
realm_handoff(ZNotice_t *notice,
	      int auth,
	      struct sockaddr_in *who,
	      ZRealm *realm,
	      int ack_to_sender)
{
#ifdef HAVE_KRB5
    Code_t retval;

    if (!auth) {
	zdbug((LOG_DEBUG, "realm_sendit unauthentic to realm %s", 
	       realm->name));
	realm_sendit(notice, who, auth, realm, ack_to_sender);
	return;
    }
  
    if (!ticket_lookup(realm->name))
	if ((retval = ticket_retrieve(realm)) != ZERR_NONE) {
	    syslog(LOG_WARNING, "rlm_handoff failed: %s", 
		   error_message(retval));
	    realm_sendit(notice, who, auth, realm, ack_to_sender);
	    return;
	}
    
    zdbug((LOG_DEBUG, "realm_sendit to realm %s auth %d", realm->name, auth)); 
    /* valid ticket available now, send the message */
    retval = realm_sendit_auth(notice, who, auth, realm, ack_to_sender);
#else /* HAVE_KRB4 */
    realm_sendit(notice, who, auth, realm, ack_to_sender);
#endif /* HAVE_KRB4 */
}

static void
realm_sendit(ZNotice_t *notice,
	     struct sockaddr_in *who,
	     int auth,
	     ZRealm *realm,
	     int ack_to_sender)
{
    caddr_t pack;
    int packlen;
    Code_t retval;
    Unacked *nacked;

    notice->z_auth = auth;
  
    /* format the notice */
    if ((retval = ZFormatRawNotice(notice, &pack, &packlen)) != ZERR_NONE) {
	syslog(LOG_WARNING, "rlm_sendit format: %s",
	       error_message(retval));
	return;
    }
  
    /* now send */
    if ((retval = ZSetDestAddr(&realm->addrs[realm->idx])) != ZERR_NONE) {
	syslog(LOG_WARNING, "rlm_sendit set addr: %s",
	       error_message(retval));
	free(pack);
	return;
    }
    if ((retval = ZSendPacket(pack, packlen, 0)) != ZERR_NONE) {
	syslog(LOG_WARNING, "rlm_sendit xmit: %s", error_message(retval));
	free(pack);
	return;
    }
    
    /* now we've sent it, mark it as not ack'ed */
  
    if (!(nacked = (Unacked *)malloc(sizeof(Unacked)))) {
	/* no space: just punt */
	syslog(LOG_ERR, "rlm_sendit nack malloc");
	free(pack);
	return;
    }

    nacked->client = NULL;
    nacked->rexmits = 0;
    nacked->packet = pack;
    nacked->dest.rlm.rlm_idx = realm - otherrealms;
    nacked->dest.rlm.rlm_srv_idx = realm->idx;
    nacked->packsz = packlen;
    nacked->uid = notice->z_uid;
    if (ack_to_sender)
	nacked->ack_addr = *who;
    else
	nacked->ack_addr.sin_addr.s_addr = 0;

    /* set a timer to retransmit */
    nacked->timer = timer_set_rel(rexmit_times[0], rlm_rexmit, nacked);
    /* chain in */
    Unacked_insert(&rlm_nacklist, nacked);
    return;
}

static void
packet_ctl_nack(Unacked *nackpacket)
{
    ZNotice_t notice;

    /* extract the notice */
    ZParseNotice(nackpacket->packet, nackpacket->packsz, &notice);
    if (nackpacket->ack_addr.sin_addr.s_addr != 0)
	nack(&notice, &nackpacket->ack_addr);
    else
	syslog(LOG_WARNING, "would have acked nobody (%s/%s/%s)",
	       notice.z_class, notice.z_class_inst, notice.z_opcode); /* XXX */
}

static void
rlm_rexmit(void *arg)
{
    Unacked *nackpacket = (Unacked *) arg;
    Code_t retval;
    register ZRealm *realm;

    zdbug((LOG_DEBUG,"rlm_rexmit"));

    realm = &otherrealms[nackpacket->dest.rlm.rlm_idx];

    zdbug((LOG_DEBUG, "rlm_rexmit: sending to %s:%d (%d)",
	   realm->name, realm->idx, nackpacket->rexmits));

    if (realm->count == 0)
	return;

    /* Check to see if we've retransmitted as many times as we can */
    if (nackpacket->rexmits >= (NUM_REXMIT_TIMES * realm->count)) {
	/* give a server ack that the packet is lost/realm dead */
	packet_ctl_nack(nackpacket);
	Unacked_delete(nackpacket);
	
	zdbug((LOG_DEBUG, "rlm_rexmit: %s appears dead", realm->name));
	realm->state = REALM_DEAD;

	free(nackpacket->packet);
	free(nackpacket);
	return;
    }

    /* if we've reached our limit, move on to the next server */
    if ((realm->state == REALM_TARDY) || 
	(nackpacket->rexmits && 
	 !((nackpacket->rexmits+1) % (NUM_REXMIT_TIMES/3)))) 
    {
	realm->idx = (realm->idx + 1) % realm->count;
	zdbug((LOG_DEBUG, "rlm_rexmit: %s switching servers:%d (%s)", 
	       realm->name, realm->idx, 
	       inet_ntoa((realm->addrs[realm->idx]).sin_addr)));
    }

    /* throttle back if it looks like the realm is down */
    if ((realm->state != REALM_DEAD) || 
	((nackpacket->rexmits % (realm->count+1)) == 1)) {
	/* do the retransmit */
	retval = ZSetDestAddr(&realm->addrs[realm->idx]);
	if (retval != ZERR_NONE) {
	    syslog(LOG_WARNING, "rlm_rexmit set addr: %s", 
		   error_message(retval));
	} else {
	    retval = ZSendPacket(nackpacket->packet, nackpacket->packsz, 0);
	    if (retval != ZERR_NONE)
		syslog(LOG_WARNING, "rlm_rexmit xmit: %s",
		       error_message(retval));
	}
	/* no per-server nack queues for foreign realms yet, doesn't matter */
	nackpacket->dest.rlm.rlm_srv_idx = realm->idx;
	zdbug((LOG_DEBUG, "rlm_rexmit(%s): send to %s", realm->name,
	       inet_ntoa((realm->addrs[realm->idx]).sin_addr)));
    } else {
	zdbug((LOG_DEBUG, "rlm_rexmit(%s): not sending to %s", realm->name,
	       inet_ntoa((realm->addrs[realm->idx]).sin_addr)));
    }

    /* reset the timer */
    nackpacket->rexmits++;
    nackpacket->timer = 
	timer_set_rel(rexmit_times[nackpacket->rexmits%NUM_REXMIT_TIMES], 
		      rlm_rexmit, nackpacket);
    if (rexmit_times[nackpacket->rexmits%NUM_REXMIT_TIMES] == -1)
	zdbug((LOG_DEBUG, "rlm_rexmit(%s): would send at -1 to %s", 
	       realm->name, inet_ntoa((realm->addrs[realm->idx]).sin_addr)));
    
    return;
}

void
realm_dump_realms(FILE *fp)
{
    register int ii, jj;
  
    for (ii = 0; ii < nrealms; ii++) {
	(void) fprintf(fp, "%d:%s\n", ii, otherrealms[ii].name);
	for (jj = 0; jj < otherrealms[ii].count; jj++) {
	    (void) fprintf(fp, "\t%s\n",
			   inet_ntoa(otherrealms[ii].addrs[jj].sin_addr));
	}
	/* dump the subs */
	subscr_dump_subs(fp, otherrealms[ii].subs);
    }
}

#ifdef HAVE_KRB5
static Code_t
realm_sendit_auth(ZNotice_t *notice,
		  struct sockaddr_in *who,
		  int auth,
		  ZRealm *realm,
		  int ack_to_sender)
{
    char *buffer, *ptr;
    int buffer_len, hdrlen, offset, fragsize, message_len;
    int origoffset, origlen;
    Code_t retval;
    Unacked *nacked;
    char multi[64];
    ZNotice_t partnotice, newnotice;

    offset = 0;

    buffer = (char *) malloc(sizeof(ZPacket_t));
    if (!buffer) {
        syslog(LOG_ERR, "realm_sendit_auth malloc");
        return ENOMEM;                 /* DON'T put on nack list */
    }

    buffer_len = sizeof(ZPacket_t);

    newnotice = *notice;

    hdrlen = 0;
    retval = ZMakeZcodeRealmAuthentication(&newnotice, buffer, buffer_len,
					   &hdrlen, realm->name);
    if (retval != ZERR_NONE) {
	syslog(LOG_WARNING, "rlm_sendit_auth make zcksum: %s", 
	       error_message(retval));
	return (retval);
    }

    /* set the dest addr */
    retval = ZSetDestAddr(&realm->addrs[realm->idx]);
    if (retval != ZERR_NONE) {
	syslog(LOG_WARNING, "rlm_sendit_auth set addr: %s", 
	       error_message(retval));
	return (retval);
    }
  
    /* This is not terribly pretty, but it does do its job. 
     * If a packet we get that needs to get sent off to another realm is
     * too big after we slap on our authent, we refragment it further,
     * a la Z_SendFragmentedNotice. This obviates the need for what
     * used to be done in ZFormatAuthenticRealmNotice, as we do it here.
     * At some point it should be pulled back out into its own function,
     * but only the server uses it.
     */ 

    if ((notice->z_message_len+hdrlen > buffer_len) || 
	(notice->z_message_len+hdrlen > Z_MAXPKTLEN)) {

	/* Reallocate buffers inside the refragmenter */
	free(buffer);

	partnotice = *notice;

	origoffset = 0;
	origlen = notice->z_message_len;

	if (notice->z_multinotice && strcmp(notice->z_multinotice, ""))
	    if (sscanf(notice->z_multinotice, "%d/%d", &origoffset, 
		       &origlen) != 2) {
		syslog(LOG_WARNING, "rlm_sendit_auth frag: parse failed");
		return ZERR_BADFIELD;
	    }

	fragsize = Z_MAXPKTLEN-hdrlen-Z_FRAGFUDGE;

	while (offset < notice->z_message_len || !notice->z_message_len) {
	    (void) sprintf(multi, "%d/%d", offset+origoffset, origlen);
	    partnotice.z_multinotice = multi;
	    if (offset > 0) {
		(void) Z_gettimeofday(&partnotice.z_uid.tv, 
				      (struct timezone *)0);
		partnotice.z_uid.tv.tv_sec = htonl((u_long) 
						   partnotice.z_uid.tv.tv_sec);
		partnotice.z_uid.tv.tv_usec = 
		    htonl((u_long) partnotice.z_uid.tv.tv_usec);
		(void) memcpy((char *)&partnotice.z_uid.zuid_addr, &__My_addr, 
			      sizeof(__My_addr));
		partnotice.z_sender_sockaddr.ip4.sin_family = AF_INET; /* XXX */
		(void) memcpy((char *)&partnotice.z_sender_sockaddr.ip4.sin_addr,
			      &__My_addr, sizeof(__My_addr));
	    }
	    message_len = min(notice->z_message_len-offset, fragsize);
	    partnotice.z_message = notice->z_message+offset;
	    partnotice.z_message_len = message_len;

	    buffer = (char *) malloc(sizeof(ZPacket_t));
	    if (!buffer) {
		syslog(LOG_ERR, "realm_sendit_auth malloc");
		return ENOMEM;                 /* DON'T put on nack list */
	    }

	    buffer_len = sizeof(ZPacket_t);
	    
	    retval = ZMakeZcodeRealmAuthentication(&partnotice, buffer, 
						   buffer_len, &hdrlen, 
						   realm->name);
	    if (retval != ZERR_NONE) {
		syslog(LOG_WARNING, "rlm_sendit_auth set addr: %s", 
		       error_message(retval));
		free(buffer);
		return (retval);
	    }

	    ptr = buffer+hdrlen;

	    (void) memcpy(ptr, partnotice.z_message, partnotice.z_message_len);

	    buffer_len = hdrlen+partnotice.z_message_len;

	    /* now send */
	    if ((retval = ZSendPacket(buffer, buffer_len, 0)) != ZERR_NONE) {
		syslog(LOG_WARNING, "rlm_sendit_auth xmit: %s", 
		       error_message(retval));
		free(buffer);
		return(retval);
	    }

	    if (!(nacked = (Unacked *)malloc(sizeof(Unacked)))) {
		/* no space: just punt */
		syslog(LOG_ERR, "rlm_sendit_auth nack malloc");
		free(buffer);
		return ENOMEM;
	    }

	    nacked->rexmits = 0;
	    nacked->packet = buffer;
	    nacked->dest.rlm.rlm_idx = realm - otherrealms;
	    nacked->dest.rlm.rlm_srv_idx = realm->idx;
	    nacked->packsz = buffer_len;
	    nacked->uid = partnotice.z_uid;

	    /* Do the ack for the last frag, below */
	    if (ack_to_sender)
		nacked->ack_addr = *who;
	    else
		nacked->ack_addr.sin_addr.s_addr = 0;

	    /* set a timer to retransmit */
	    nacked->timer = timer_set_rel(rexmit_times[0], rlm_rexmit, nacked);

	    /* chain in */
	    Unacked_insert(&rlm_nacklist, nacked);

	    offset += fragsize;
	    
	    if (!notice->z_message_len)
		break;
	}
    }
    else {
	/* This is easy, no further fragmentation needed */
	ptr = buffer+hdrlen;

	(void) memcpy(ptr, newnotice.z_message, newnotice.z_message_len);

        buffer_len = hdrlen+newnotice.z_message_len;
    
	/* now send */
	if ((retval = ZSendPacket(buffer, buffer_len, 0)) != ZERR_NONE) {
	    syslog(LOG_WARNING, "rlm_sendit_auth xmit: %s", 
		   error_message(retval));
	    free(buffer);
	    return(retval);
	}

	/* now we've sent it, mark it as not ack'ed */
    
	if (!(nacked = (Unacked *)malloc(sizeof(Unacked)))) {
	    /* no space: just punt */
	    syslog(LOG_ERR, "rlm_sendit_auth nack malloc");
	    free(buffer);
	    return 0;
	}

	nacked->rexmits = 0;
	nacked->packet = buffer;
	nacked->dest.rlm.rlm_idx = realm - otherrealms;
	nacked->dest.rlm.rlm_srv_idx = realm->idx;
	nacked->packsz = buffer_len;
	nacked->uid = notice->z_uid;
	
	/* Do the ack for the last frag, below */
	if (ack_to_sender)
	    nacked->ack_addr = *who;
	else
	    nacked->ack_addr.sin_addr.s_addr = 0;
    
	/* set a timer to retransmit */
	nacked->timer = timer_set_rel(rexmit_times[0], rlm_rexmit, nacked);
	/* chain in */
	Unacked_insert(&rlm_nacklist, nacked);
    }
    return 0;
}

static int
ticket_lookup(char *realm)
{
    krb5_error_code result;
    krb5_timestamp sec;
    krb5_ccache ccache; 
    krb5_creds creds_in, *creds; 

    result = krb5_cc_default(Z_krb5_ctx, &ccache); 
    if (result) 
      return 0;

    memset(&creds_in, 0, sizeof(creds_in)); 
 
    result = krb5_cc_get_principal(Z_krb5_ctx, ccache, &creds_in.client); 
    if (result) {
      krb5_cc_close(Z_krb5_ctx, ccache);
      return 0;
    }

    result = krb5_build_principal(Z_krb5_ctx, &creds_in.server, 
                                  strlen(realm), 
                                  realm, 
                                  SERVER_KRB5_SERVICE, SERVER_INSTANCE,
				  NULL); 
    if (result) {
      krb5_cc_close(Z_krb5_ctx, ccache);
      return 0;
    }

    result = krb5_get_credentials(Z_krb5_ctx, 0 /* flags */, ccache, 
                                  &creds_in, &creds); 
    krb5_cc_close(Z_krb5_ctx, ccache);
    /* good ticket? */

    krb5_timeofday (Z_krb5_ctx, &sec);
    krb5_free_cred_contents(Z_krb5_ctx, &creds_in); /* hope this is OK */ 
    if ((result == 0) && (sec < creds->times.endtime)) {
      krb5_free_creds(Z_krb5_ctx, creds);
      return (1);
    }
    if (!result) krb5_free_creds(Z_krb5_ctx, creds);

    return (0);
}

static Code_t
ticket_retrieve(ZRealm *realm)
{
    int pid;
    krb5_ccache ccache;
    krb5_error_code result; 
    krb5_creds creds_in, *creds; 
    
    get_tgt();

    if (realm->child_pid) 
	/* Right idea. Basically, we haven't gotten it yet */
	return KRB5KRB_AP_ERR_TKT_EXPIRED;

    if (realm->have_tkt) {
	/* Get a pointer to the default ccache. We don't need to free this. */ 
	result = krb5_cc_default(Z_krb5_ctx, &ccache); 

	/* GRRR.  There's no allocator or constructor for krb5_creds */ 
	/* GRRR.  It would be nice if this API were documented at all */ 
	memset(&creds_in, 0, sizeof(creds_in)); 
 
	if (!result) 
	    result = krb5_cc_get_principal(Z_krb5_ctx, ccache, &creds_in.client); 
	/* construct the service principal */ 
	if (!result)  
	    result = krb5_build_principal(Z_krb5_ctx, &creds_in.server, 
					  strlen(realm->name), realm->name, 
					  SERVER_KRB5_SERVICE, SERVER_INSTANCE, 
					  NULL); 

	/* HOLDING: creds_in.server */ 
     
	/* look up or get the credentials we need */ 
	if (!result) 
	    result = krb5_get_credentials(Z_krb5_ctx, 0 /* flags */, ccache, 
					  &creds_in, &creds); 
	krb5_cc_close(Z_krb5_ctx, ccache);
	krb5_free_cred_contents(Z_krb5_ctx, &creds_in); /* hope this is OK */ 
	if (!result) {
	    krb5_free_creds(Z_krb5_ctx, creds); 
	    return 0; 
	}
    } else {
	syslog(LOG_ERR, "tkt_rtrv: don't have ticket, but have no child");
        result = KRB5KRB_AP_ERR_TKT_EXPIRED;
    }
 
    pid = fork();
    if (pid < 0) {
	syslog(LOG_ERR, "tkt_rtrv: can't fork");
	return errno;
    }
    else if (pid == 0) {
#ifdef _POSIX_VERSION
	struct sigaction action;

	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);
	action.sa_handler = 0;
	sigaction(SIGCHLD, &action, NULL);
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGUSR1, &action, NULL);
	sigaction(SIGUSR2, &action, NULL);
	sigaction(SIGFPE, &action, NULL);
	sigaction(SIGHUP, &action, NULL);
#ifdef SIGEMT
	sigaction(SIGEMT, &action, NULL);
#endif
#else
	signal(SIGCHLD, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGUSR1, SIG_DFL);
	signal(SIGUSR2, SIG_DFL);
	signal(SIGFPE, SIG_DFL);
	signal(SIGHUP, SIG_DFL);
#ifdef SIGEMT
	signal(SIGEMT, SIG_DFL);
#endif
#endif

	syslog(LOG_INFO, "tkt_rtrv running for %s", realm->name);
	while (1) {
	    /* Get a pointer to the default ccache.
	       We don't need to free this. */ 
	    result = krb5_cc_default(Z_krb5_ctx, &ccache); 

	    /* GRRR.  There's no allocator or constructor for krb5_creds */ 
	    /* GRRR.  It would be nice if this API were documented at all */ 
	    memset(&creds_in, 0, sizeof(creds_in)); 
 
	    if (!result) 
		result = krb5_cc_get_principal(Z_krb5_ctx, ccache,
					       &creds_in.client); 
	    /* construct the service principal */ 
	    if (!result)  
		result = krb5_build_principal(Z_krb5_ctx, &creds_in.server, 
					      strlen(realm->name), realm->name, 
					      SERVER_KRB5_SERVICE,
					      SERVER_INSTANCE, 
					      NULL); 

	    /* HOLDING: creds_in.server */ 
	    
	    /* look up or get the credentials we need */ 
	    if (!result) 
		result = krb5_get_credentials(Z_krb5_ctx, 0 /* flags */, ccache, 
					      &creds_in, &creds); 
	    krb5_cc_close(Z_krb5_ctx, ccache);
	    krb5_free_cred_contents(Z_krb5_ctx, &creds_in); /* hope this is OK */ 
	    if (!result) {
		krb5_free_creds(Z_krb5_ctx, creds); 
		syslog(LOG_INFO, "tkt_rtrv succeeded for %s", realm->name);
		exit(0);
	    }
      
	    /* Sleep a little while before retrying */
	    sleep(30);
	}
    } else {
	realm->child_pid = pid;
	realm->have_tkt = 0;

	syslog(LOG_WARNING, "tkt_rtrv: %s: %d", realm->name,
	       result);
	return (result);
    }
}
#endif /* HAVE_KRB5 */

