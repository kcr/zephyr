/*
 * $Id: kopt.c,v 1.15 2000-05-17 04:30:49 ghudson Exp $
 *
 * Copyright 1985, 1986, 1987, 1988, 1990, 1991 by the Massachusetts
 * Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 */

/*
 * This includes code taken from:
 * Kerberos: rd_req.c,v 4.16 89/03/22 14:52:06 jtkohl Exp
 * Kerberos: prot.h,v 4.13 89/01/24 14:27:22 jtkohl Exp
 * Kerberos: krb_conf.h,v 4.0 89/01/23 09:59:27 jtkohl Exp
 */

#include <zephyr/mit-copyright.h>
#include "zserver.h"

#ifndef lint
#ifndef SABER
static const char *rcsid_rd_req_c =
    "$Id: kopt.c,v 1.15 2000-05-17 04:30:49 ghudson Exp $";
#endif /* lint */
#endif /* SABER */

#ifdef HAVE_KRB4
#ifndef NOENCRYPTION

/* Byte ordering */
#undef HOST_BYTE_ORDER
static int krbONE = 1;
#define		HOST_BYTE_ORDER	(* (char *) &krbONE)

#define		KRB_PROT_VERSION 	4

/* Message types , always leave lsb for byte order */

#define		AUTH_MSG_KDC_REQUEST			 1<<1
#define 	AUTH_MSG_KDC_REPLY			 2<<1
#define		AUTH_MSG_APPL_REQUEST			 3<<1
#define		AUTH_MSG_APPL_REQUEST_MUTUAL		 4<<1
#define		AUTH_MSG_ERR_REPLY			 5<<1
#define		AUTH_MSG_PRIVATE			 6<<1
#define		AUTH_MSG_SAFE				 7<<1
#define		AUTH_MSG_APPL_ERR			 8<<1
#define 	AUTH_MSG_DIE				63<<1

/* values for kerb error codes */

#define		KERB_ERR_OK				 0
#define		KERB_ERR_NAME_EXP			 1
#define		KERB_ERR_SERVICE_EXP			 2
#define		KERB_ERR_AUTH_EXP			 3
#define		KERB_ERR_PKT_VER			 4
#define		KERB_ERR_NAME_MAST_KEY_VER		 5
#define		KERB_ERR_SERV_MAST_KEY_VER		 6
#define		KERB_ERR_BYTE_ORDER			 7
#define		KERB_ERR_PRINCIPAL_UNKNOWN		 8
#define		KERB_ERR_PRINCIPAL_NOT_UNIQUE		 9
#define		KERB_ERR_NULL_KEY			10

extern int krb_ap_req_debug;

extern struct timeval t_local;

/*
 * Keep the following information around for subsequent calls
 * to this routine by the same server using the same key.
 */

static Sched serv_ksched;	/* Key sched to decrypt ticket */
static des_cblock serv_key;	/* Initialization vector */

static int st_kvno;		/* version number for this key */
static char st_rlm[REALM_SZ];	/* server's realm */
static char st_nam[ANAME_SZ];	/* service name */
static char st_inst[INST_SZ];	/* server's instance */

/*
 * Cache of key schedules
 */
#define HASH_SIZE_1	255	/* not a power of 2 */
#define HASH_SIZE_2	3
static unsigned long last_use;
typedef struct {
    unsigned long last_time_used;
    des_cblock key;
    Sched schedule;
} KeySchedRec;
static KeySchedRec scheds[HASH_SIZE_1][HASH_SIZE_2];

Sched *check_key_sched_cache(key)
    des_cblock key;
{
    unsigned int hash_value = key[0] + key[1] * 256;
    KeySchedRec *rec = scheds[hash_value % HASH_SIZE_1];
    int i;

    for (i = HASH_SIZE_2 - 1; i >= 0; i--) {
	if (rec[i].last_time_used && key[0] == rec[i].key[0]
	    && !memcmp(key, rec[i].key, sizeof(des_cblock))) {
	    rec[i].last_time_used = last_use++;
	    return &rec[i].schedule;
	}
    }
    return 0;
}

void add_to_key_sched_cache(key, sched)
    des_cblock key;
    Sched *sched;
{
    unsigned int hash_value = key[0] + key[1] * 256;
    KeySchedRec *rec = scheds[hash_value % HASH_SIZE_1];
    int i, oldest = HASH_SIZE_2 - 1;

    for (i = HASH_SIZE_2 - 1; i >= 0; i--) {
	if (rec[i].last_time_used == 0) {
	    oldest = i;
	    break;
	}
	if (rec[i].last_time_used < rec[oldest].last_time_used)
	    oldest = i;
    }
    memcpy (rec[oldest].key, key, sizeof(des_cblock));
    rec[oldest].schedule = *sched;
    rec[oldest].last_time_used = last_use++;
}

/*
 * This file contains two functions.  krb_set_key() takes a DES
 * key or password string and returns a DES key (either the original
 * key, or the password converted into a DES key) and a key schedule
 * for it.
 *
 * krb_rd_req() reads an authentication request and returns information
 * about the identity of the requestor, or an indication that the
 * identity information was not authentic.
 */

/*
 * krb_set_key() takes as its first argument either a DES key or a
 * password string.  The "cvt" argument indicates how the first
 * argument "key" is to be interpreted: if "cvt" is null, "key" is
 * taken to be a DES key; if "cvt" is non-null, "key" is taken to
 * be a password string, and is converted into a DES key using
 * string_to_key().  In either case, the resulting key is returned
 * in the external variable "serv_key".  A key schedule is
 * generated for "serv_key" and returned in the external variable
 * "serv_ksched".
 *
 * This routine returns the return value of des_key_sched.
 *
 * krb_set_key() needs to be in the same .o file as krb_rd_req() so that
 * the key set by krb_set_key() is available in private storage for
 * krb_rd_req().
 */

int
krb_set_key(key,cvt)
    char *key;
    int cvt;
{
#ifdef NOENCRYPTION
    memset(serv_key, 0, sizeof(serv_key));
    return KSUCCESS;
#else /* Encrypt */
    Sched *s;
    int ret;

    if (cvt)
	string_to_key(key,serv_key);
    else
	memcpy((char *)serv_key,key,8);

    s = check_key_sched_cache (serv_key);
    if (s) {
	serv_ksched = *s;
	return 0;
    }
    ret = des_key_sched(serv_key, serv_ksched.s);
    add_to_key_sched_cache(serv_key, &serv_ksched);
    return ret;
#endif /* NOENCRYPTION */
}


/*
 * krb_rd_req() takes an AUTH_MSG_APPL_REQUEST or
 * AUTH_MSG_APPL_REQUEST_MUTUAL message created by krb_mk_req(),
 * checks its integrity and returns a judgement as to the requestor's
 * identity.
 *
 * The "authent" argument is a pointer to the received message.
 * The "service" and "instance" arguments name the receiving server,
 * and are used to get the service's ticket to decrypt the ticket
 * in the message, and to compare against the server name inside the
 * ticket.  "from_addr" is the network address of the host from which
 * the message was received; this is checked against the network
 * address in the ticket.  If "from_addr" is zero, the check is not
 * performed.  "ad" is an AUTH_DAT structure which is
 * filled in with information about the sender's identity according
 * to the authenticator and ticket sent in the message.  Finally,
 * "fn" contains the name of the file containing the server's key.
 * (If "fn" is NULL, the server's key is assumed to have been set
 * by krb_set_key().  If "fn" is the null string ("") the default
 * file KEYFILE, defined in "krb.h", is used.)
 *
 * krb_rd_req() returns RD_AP_OK if the authentication information
 * was genuine, or one of the following error codes (defined in
 * "krb.h"):
 *
 *	RD_AP_VERSION		- wrong protocol version number
 *	RD_AP_MSG_TYPE		- wrong message type
 *	RD_AP_UNDEC		- couldn't decipher the message
 *	RD_AP_INCON		- inconsistencies found
 *	RD_AP_BADD		- wrong network address
 *	RD_AP_TIME		- client time (in authenticator)
 *				  too far off server time
 *	RD_AP_NYV		- Kerberos time (in ticket) too
 *				  far off server time
 *	RD_AP_EXP		- ticket expired
 *
 * For the message format, see krb_mk_req().
 *
 * Mutual authentication is not implemented.
 */

int
krb_rd_req(authent,service,instance,from_addr,ad,fn)
    KTEXT authent;			/* The received message */
    char *service;			/* Service name */
    char *instance;			/* Service instance */
    unsigned KRB_INT32 from_addr;	/* Net address of originating host */
    AUTH_DAT *ad;			/* Structure to be filled in */
    char *fn;				/* Filename to get keys from */
{
    KTEXT_ST ticket;     /* Temp storage for ticket */
    KTEXT tkt = &ticket;
    KTEXT_ST req_id_st;  /* Temp storage for authenticator */
    KTEXT req_id = &req_id_st;

    char realm[REALM_SZ];	/* Realm of issuing kerberos */
    Sched seskey_sched, *sched;	/* Key sched for session key */
    unsigned char skey[KKEY_SZ]; /* Session key from ticket */
    char sname[SNAME_SZ];	/* Service name from ticket */
    char iname[INST_SZ];	/* Instance name from ticket */
    char r_aname[ANAME_SZ];	/* Client name from authenticator */
    char r_inst[INST_SZ];	/* Client instance from authenticator */
    char r_realm[REALM_SZ];	/* Client realm from authenticator */
    unsigned int r_time_ms;     /* Fine time from authenticator */
    unsigned long r_time_sec;   /* Coarse time from authenticator */
    char *ptr;			/* For stepping through */
    unsigned long delta_t;      /* Time in authenticator - local time */
    long tkt_age;		/* Age of ticket */
    int swap_bytes;		/* Need to swap bytes? */
    int mutual;			/* Mutual authentication requested? */
    unsigned char s_kvno;	/* Version number of the server's key
				 * Kerberos used to encrypt ticket */
    int status;

    if (authent->length <= 0)
	return(RD_AP_MODIFIED);

    ptr = (char *) authent->dat;

    /* get msg version, type and byte order, and server key version */

    /* check version */
    if (KRB_PROT_VERSION != (unsigned int) *ptr++)
        return RD_AP_VERSION;

    /* byte order */
    swap_bytes = 0;
    if ((*ptr & 1) != HOST_BYTE_ORDER)
        swap_bytes++;

    /* check msg type */
    mutual = 0;
    switch (*ptr++ & ~1) {
      case AUTH_MSG_APPL_REQUEST:
        break;
      case AUTH_MSG_APPL_REQUEST_MUTUAL:
        mutual++;
        break;
      default:
        return(RD_AP_MSG_TYPE);
    }

#ifdef lint
    /* XXX mutual is set but not used; why??? */
    /* this is a crock to get lint to shut up */
    if (mutual)
        mutual = 0;
#endif /* lint */
    s_kvno = *ptr++;		/* get server key version */
    (void) strncpy(realm,ptr,REALM_SZ);		/* And the realm of the issuing KDC */
    realm[REALM_SZ-1] = '\0';
    ptr += strlen(realm) + 1;     /* skip the realm "hint" */

    /*
     * If "fn" is NULL, key info should already be set; don't
     * bother with ticket file.  Otherwise, check to see if we
     * already have key info for the given server and key version
     * (saved in the static st_* variables).  If not, go get it
     * from the ticket file.  If "fn" is the null string, use the
     * default ticket file.
     */
    if (fn && (strcmp(st_nam,service) != 0 || strcmp(st_inst,instance) != 0 ||
               strcmp(st_rlm,realm) != 0 || (st_kvno != s_kvno))) {
        if (*fn == 0)
	    fn = KEYFILE;
        st_kvno = s_kvno;
#ifndef NOENCRYPTION
        if (read_service_key(service,instance,realm, (int) s_kvno,
                            fn, (char *) skey))
            return(RD_AP_UNDEC);
        status = krb_set_key((char *) skey, 0);
	if (status != 0)
	    return(status);
#endif /* !NOENCRYPTION */
        strcpy(st_rlm,realm);
        strcpy(st_nam,service);
        strcpy(st_inst,instance);
    }

    /* Get ticket from authenticator */
    tkt->length = (int) *ptr++;
    if ((tkt->length + (ptr+1 - (char *) authent->dat)) > authent->length)
	return RD_AP_MODIFIED;
    memcpy(tkt->dat, ptr + 1, tkt->length);

    if (krb_ap_req_debug)
        krb_log("ticket->length: %d", tkt->length);

#ifndef NOENCRYPTION
    /* Decrypt and take apart ticket */
#endif

    if (decomp_ticket(tkt, &ad->k_flags, ad->pname, ad->pinst, ad->prealm,
                      &(ad->address), ad->session, &(ad->life),
                      &(ad->time_sec), sname, iname, serv_key, serv_ksched.s))
        return RD_AP_UNDEC;

    if (krb_ap_req_debug) {
        krb_log("Ticket Contents.");
        krb_log(" Aname:   %s.%s",ad->pname,
                ((int)*(ad->prealm) ? ad->prealm : "Athena"));
        krb_log(" Service: %s%s%s", sname, ((int)*iname ? "." : ""), iname);
    }

    /* Extract the authenticator */
    req_id->length = (int) *(ptr++);
    if ((req_id->length + (ptr + tkt->length - (char *) authent->dat)) >
	authent->length)
	return RD_AP_MODIFIED;
    memcpy(req_id->dat, ptr + tkt->length, req_id->length);

#ifndef NOENCRYPTION
    /* And decrypt it with the session key from the ticket */
    if (krb_ap_req_debug)
	krb_log("About to decrypt authenticator");
    sched = check_key_sched_cache(ad->session);
    if (!sched) {
	sched = &seskey_sched;
	key_sched(ad->session, seskey_sched.s);
	add_to_key_sched_cache(ad->session, &seskey_sched);
    }
    /* can't do much to optimize this... */
    pcbc_encrypt((C_Block *) req_id->dat, (C_Block *) req_id->dat,
		 (long) req_id->length, sched->s, ad->session, DES_DECRYPT);
    if (krb_ap_req_debug)
	krb_log("Done.");
#endif /* NOENCRYPTION */

#define check_ptr() if ((ptr - (char *) req_id->dat) > req_id->length) return(RD_AP_MODIFIED);

    ptr = (char *) req_id->dat;
    (void) strncpy(r_aname,ptr,ANAME_SZ);	/* Authentication name */
    r_aname[ANAME_SZ-1] = '\0';
    ptr += strlen(r_aname) + 1;
    check_ptr();
    (void) strncpy(r_inst,ptr,INST_SZ);		/* Authentication instance */
    r_inst[INST_SZ-1] = '\0';
    ptr += strlen(r_inst) + 1;
    check_ptr();
    (void) strncpy(r_realm,ptr,REALM_SZ);	/* Authentication name */
    r_realm[REALM_SZ-1] = '\0';
    ptr += strlen(r_realm) + 1;
    check_ptr();
    memcpy(&ad->checksum, ptr, 4);	/* Checksum */
    ptr += 4;
    check_ptr();
    if (swap_bytes)
	swap_u_long(ad->checksum);
    r_time_ms = *(ptr++);	/* Time (fine) */
#ifdef lint
    /* XXX r_time_ms is set but not used.  why??? */
    /* this is a crock to get lint to shut up */
    if (r_time_ms)
        r_time_ms = 0;
#endif /* lint */
    check_ptr();
    /* assume sizeof(r_time_sec) == 4 ?? */
    memcpy(&r_time_sec,ptr,4); /* Time (coarse) */
    if (swap_bytes)
	swap_u_long(r_time_sec);

    /* Check for authenticity of the request */
    if (krb_ap_req_debug)
        krb_log("Pname:   %s %s",ad->pname,r_aname);
    if (strcmp(ad->pname,r_aname) != 0)
        return RD_AP_INCON;
    if (strcmp(ad->pinst,r_inst) != 0)
        return RD_AP_INCON;
    if (krb_ap_req_debug)
        krb_log("Realm:   %s %s", ad->prealm, r_realm);
    if (strcmp(ad->prealm,r_realm) != 0)
        return RD_AP_INCON;

    if (krb_ap_req_debug)
        krb_log("Address: %d %d", ad->address, from_addr);
    if (from_addr && (ad->address != from_addr))
        return RD_AP_BADD;

    delta_t = abs((int)(t_local.tv_sec - r_time_sec));
    if (delta_t > CLOCK_SKEW) {
	gettimeofday(&t_local, NULL);
	delta_t = abs((int)(t_local.tv_sec - r_time_sec));
	if (delta_t > CLOCK_SKEW) {
	    if (krb_ap_req_debug) {
		krb_log("Time out of range: %d - %d = %d",
			t_local.tv_sec, r_time_sec, delta_t);
	    }
	    return RD_AP_TIME;
	}
    }

    /* Now check for expiration of ticket */

    tkt_age = t_local.tv_sec - ad->time_sec;
    if (krb_ap_req_debug) {
        krb_log("Time: %d Issue Date: %d Diff: %d Life %x",
                t_local.tv_sec, ad->time_sec, tkt_age, ad->life);
    }

    if (t_local.tv_sec < ad->time_sec) {
        if (ad->time_sec - t_local.tv_sec > CLOCK_SKEW)
            return RD_AP_NYV;
    } else if (t_local.tv_sec - ad->time_sec > 5 * 60 * ad->life) {
        return RD_AP_EXP;
    }

    /* All seems OK */
    ad->reply.length = 0;

    return RD_AP_OK;
}
#endif /* NOENCRYPTION */

int
krb_find_ticket(authent, ticket)
    KTEXT authent, ticket;
{
    char *ptr;		/* For stepping through */

    /* Check for bogus length. */
    if (authent->length <= 0)
	return RD_AP_MODIFIED;

    ptr = (char *) authent->dat;

    /* check version */
    if (KRB_PROT_VERSION != (unsigned int) *ptr++)
        return RD_AP_VERSION;

    /* Make sure msg type is ok. */
    switch (*ptr++ & ~1) {
    case AUTH_MSG_APPL_REQUEST:
    case AUTH_MSG_APPL_REQUEST_MUTUAL:
        break;
    default:
        return RD_AP_MSG_TYPE;
    }

    *ptr++;			/* skip server key version */
    ptr += strlen(ptr) + 1;     /* skip the realm "hint" */

    /* Get ticket from authenticator */
    ticket->length = (int) *ptr++;
    if ((ticket->length + (ptr + 1 - (char *) authent->dat)) > authent->length)
	return RD_AP_MODIFIED;
    memcpy((char *)(ticket->dat),ptr+1,ticket->length);

    return RD_AP_OK;
}

static char local_realm_buffer[REALM_SZ+1];

int
krb_get_lrealm(r,n)
    char *r;
    int n;
{
    FILE *cnffile, *fopen();

    if (n > 1)
	return KFAILURE;  /* Temporary restriction */

    if (my_realm[0]) {
	strcpy(r, my_realm);
	return KSUCCESS;
    }

    if (local_realm_buffer[0]) {
	strcpy(r, local_realm_buffer);
	return KSUCCESS;
    }
    
    cnffile = fopen(KRB_CONF, "r");
    if (cnffile == NULL) {
	if (n == 1) {
	    strcpy(r, KRB_REALM);
	    return KSUCCESS;
	} else {
	    return KFAILURE;
	}
    }

    if (fscanf(cnffile,"%s",r) != 1) {
        fclose(cnffile);
        return KFAILURE;
    }
    fclose(cnffile);
    return KSUCCESS;
}

int
decomp_ticket(tkt, flags, pname, pinstance, prealm, paddress, session,
              life, time_sec, sname, sinstance, key, key_s)
    KTEXT tkt;                  /* The ticket to be decoded */
    unsigned char *flags;       /* Kerberos ticket flags */
    char *pname;                /* Authentication name */
    char *pinstance;            /* Principal's instance */
    char *prealm;               /* Principal's authentication domain */
    unsigned long *paddress; /* Net address of entity
                                 * requesting ticket */
    C_Block session;            /* Session key inserted in ticket */
    int *life;                  /* Lifetime of the ticket */
    unsigned long *time_sec; /* Issue time and date */
    char *sname;                /* Service name */
    char *sinstance;            /* Service instance */
    C_Block key;                /* Service's secret key
                                 * (to decrypt the ticket) */
    des_key_schedule key_s;	/* The precomputed key schedule */
{
    static int tkt_swap_bytes;
    unsigned char *uptr;
    char *ptr = (char *)tkt->dat;

#ifndef NOENCRYPTION
    /* Do the decryption */
    pcbc_encrypt((C_Block *)tkt->dat,(C_Block *)tkt->dat,
                 (long) tkt->length,key_s,(C_Block *) key,0);
#endif /* ! NOENCRYPTION */

    *flags = *ptr;              /* get flags byte */
    ptr += sizeof(*flags);
    tkt_swap_bytes = 0;
    if (HOST_BYTE_ORDER != ((*flags >> K_FLAG_ORDER)& 1))
        tkt_swap_bytes++;

    if (strlen(ptr) > ANAME_SZ)
        return(KFAILURE);
    strcpy(pname,ptr);   /* pname */
    ptr += strlen(pname) + 1;

    if (strlen(ptr) > INST_SZ)
        return(KFAILURE);
    strcpy(pinstance,ptr); /* instance */
    ptr += strlen(pinstance) + 1;

    if (strlen(ptr) > REALM_SZ)
        return(KFAILURE);
    strcpy(prealm,ptr);  /* realm */
    ptr += strlen(prealm) + 1;
    /* temporary hack until realms are dealt with properly */
    if (*prealm == 0)
	strcpy(prealm, ZGetRealm());

    memcpy((char *)paddress, ptr, 4); /* net address */
    ptr += 4;

    memcpy((char *)session, ptr, 8); /* session key */
    ptr+= 8;

    /* get lifetime, being certain we don't get negative lifetimes */
    uptr = (unsigned char *) ptr++;
    *life = (int) *uptr;

    memcpy((char *) time_sec, ptr, 4); /* issue time */
    ptr += 4;
    if (tkt_swap_bytes)
	swap_u_long(*time_sec);

    strcpy(sname,ptr);   /* service name */
    ptr += 1 + strlen(sname);

    strcpy(sinstance,ptr); /* instance */
    ptr += 1 + strlen(sinstance);

    return(KSUCCESS);
}
#endif /* HAVE_KRB4 */

