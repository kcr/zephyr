#ifdef KERBEROS
/* This file is part of the Project Athena Zephyr Notification System.
 * It contains functions for dealing with Kerberos functions in the server.
 *
 *	Created by:	John T Kohl
 *
 *	Copyright (c) 1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/*
 *	$Source: /srv/kcr/locker/zephyr/server/kstuff.c,v $
 *	$Header: /srv/kcr/locker/zephyr/server/kstuff.c,v 1.13 1991-12-04 13:25:47 lwvanels Exp $
 */

#ifndef lint
#ifndef SABER
static char rcsid_kstuff_c[] = "$Id: kstuff.c,v 1.13 1991-12-04 13:25:47 lwvanels Exp $";
#endif
#endif

#include "zserver.h"

#include <ctype.h>
#include <netdb.h>
#include <strings.h>
#include <zephyr/zephyr_internal.h>

static char tkt_file[] = ZEPHYR_TKFILE;


struct AuthEnt {
    Zconst char *data;
    int len;
    ZSTRING *principal;
#ifndef NOENCRYPTION
    C_Block session_key;
#endif
    long expire_time;
    struct sockaddr_in from;
};

#define HASH_SIZE_1	513
#define HASH_SIZE_2	3
static struct AuthEnt auth_cache[HASH_SIZE_1][HASH_SIZE_2];

/*
 * GetKerberosData
 *
 * get ticket from file descriptor and decode it.
 * Return KFAILURE if we barf on reading the ticket, else return
 * the value of rd_ap_req() applied to the ticket.
 */
int
GetKerberosData(fd, haddr, kdata, service, srvtab)
     int fd; /* file descr. to read from */
     struct in_addr haddr; /* address of foreign host on fd */
     AUTH_DAT *kdata;	/* kerberos data (returned) */
     char *service; /* service principal desired */
     char *srvtab; /* file to get keys from */
{
	char p[20];
	KTEXT_ST ticket;	/* will get Kerberos ticket from client */
	int i;
	char instance[INST_SZ];

	/*
	 * Get the Kerberos ticket.  The first few characters, terminated
	 * by a blank, should give us a length; then get than many chars
	 * which will be the ticket proper.
	 */
	for (i=0; i<20; i++) {
		if (read(fd, &p[i], 1) != 1) {
			syslog(LOG_WARNING,"bad read tkt len");
			return(KFAILURE);
		}
		if (p[i] == ' ') {
		    p[i] = '\0';
		    break;
		}
	}
	ticket.length = atoi(p);
	if ((i==20) || (ticket.length<=0) || (ticket.length>MAX_KTXT_LEN)) {
		syslog(LOG_WARNING,"bad tkt len %d",ticket.length);
	    return(KFAILURE);
	}
	for (i=0; i<ticket.length; i++) {
	    if (read(fd, (caddr_t) &(ticket.dat[i]), 1) != 1) {
		syslog(LOG_WARNING,"bad tkt read");
		return(KFAILURE);
	    }
	}
	/*
	 * now have the ticket.  use it to get the authenticated
	 * data from Kerberos.
	 */
	(void) strcpy(instance,"*");	/* let Kerberos fill it in */

	return(krb_rd_req(&ticket,service,instance,haddr,kdata, srvtab ? srvtab : ""));
}

/*
 * SendKerberosData
 * 
 * create and transmit a ticket over the file descriptor for service.host
 * return failure codes if appropriate, or 0 if we
 * get the ticket and write it to the file descriptor
 */

int
SendKerberosData(fd, ticket, service, host)
     int fd;	/* file descriptor to write onto */
     KTEXT ticket;	/* where to put ticket (return) */
     char *service;	/* service name, foreign host */
     char *host;
{
    int rem;
    char p[32];
    char krb_realm[REALM_SZ];
    int written;
    int size_to_write;

    rem = krb_get_lrealm(krb_realm,1);
    if (rem != KSUCCESS)
	return rem + krb_err_base;

    rem = krb_mk_req( ticket, service, host, krb_realm, (u_long)0 );
    if (rem != KSUCCESS)
	return rem + krb_err_base;

    (void) sprintf(p,"%d ",ticket->length);
    size_to_write = strlen (p);
    if ((written = write(fd, p, size_to_write)) != size_to_write)
	    if (written < 0)
		    return errno;
	    else
		    return ZSRV_PKSHORT;
    if ((written = write(fd, (caddr_t) (ticket->dat), ticket->length)) != ticket->length)
	    if (written < 0)
		    return errno;
	    else
		    return ZSRV_PKSHORT;

    return 0;
}

/* Hack to replace the kerberos library's idea of the ticket file with
   our idea */
char *
tkt_string()
{
    return tkt_file;
}

/* Check authentication of the notice.
   If it looks authentic but fails the Kerberos check, return -1.
   If it looks authentic and passes the Kerberos check, return 1.
   If it doesn't look authentic, return 0
  
   When not using Kerberos, return (looks-authentic-p)
 */

static void
ae_expire(ae)
     struct AuthEnt *ae;
{
  if (ae->data) {
    xfree((void *) ae->data);
    ae->data = 0;
  }
  ae->len = 0;
  ae->expire_time = 0;
  ae->principal = 0;
}

static int
auth_hash (str, len)
     Zconst char *str;
     int len;
{
    unsigned long hash;
    if (len <= 3)
	return str[0];
    hash = str[len - 1] * 256 + str[len-2] * 16 + str[len-3];
    hash %= HASH_SIZE_1;
    return hash;
}

static int
check_cache (notice, from)
     ZNotice_t *notice;
     struct sockaddr_in *from;
 {
    Zconst char *str = notice->z_ascii_authent;
    int len, i;
    unsigned int hash_val = 0;
    unsigned long now = time(0);
    struct AuthEnt *a;

    len = strlen (str);
    hash_val = auth_hash (str, len);
    for (i = 0; i < HASH_SIZE_2; i++) {
	a = &auth_cache[hash_val][i];
	if (!a->data) {
	    continue;
	}
	if (now > a->expire_time) {
	    ae_expire(a);
	    continue;
	}
	if (len != a->len) {
	    continue;
	}
	if (strcmp (notice->z_ascii_authent, a->data)) {
	    continue;
	}
	/* Okay, we know we've got the same authenticator.  */
	if (strcmp (notice->z_sender, a->principal->string)) {
	    return ZAUTH_FAILED;
	}
	if (from->sin_addr.s_addr != a->from.sin_addr.s_addr) {
	    return ZAUTH_FAILED;
	}
#ifndef NOENCRYPTION
	bcopy (a->session_key, __Zephyr_session, sizeof (C_Block));
#endif
	return ZAUTH_YES;
    }
    return ZAUTH_NO;
}

void 
add_to_cache (a)
     struct AuthEnt *a;
{
    int len, i, j;
    struct AuthEnt *entries;
    unsigned int hash_val = 0;

    len = a->len;
    hash_val = auth_hash (a->data, len);
    entries = auth_cache[hash_val];
    j = 0;
    for (i = 0; i < HASH_SIZE_2; i++) {
	if (entries[i].data == 0) {
	    j = i;
	    goto ok;
	}
	if (i == j)
	    continue;
	if (entries[i].expire_time < entries[j].expire_time)
	    j = i;
    }
ok:
    if (entries[j].data)
	ae_expire(&entries[j]);
    entries[j] = *a;
}

int
ZCheckAuthentication(notice, from)
     ZNotice_t *notice;
     struct sockaddr_in *from;
{	
    int result;
    char srcprincipal[ANAME_SZ+INST_SZ+REALM_SZ+4];
    KTEXT_ST authent;
    AUTH_DAT dat;
    ZChecksum_t our_checksum;
    CREDENTIALS cred;
    struct AuthEnt a;
    char *s;
    int auth_len = 0;


    if (!notice->z_auth) {
	return (ZAUTH_NO);
    }

    if (__Zephyr_server) {
	
	if (notice->z_authent_len <= 0) { /* bogus length */
#if 0
	    syslog (LOG_DEBUG, "z_authent_len = %d -> AUTH_FAILED",
		    notice->z_authent_len);
#endif
	    return(ZAUTH_FAILED);
	}

	auth_len = strlen (notice->z_ascii_authent);
	if (ZReadAscii(notice->z_ascii_authent, auth_len + 1, 
		       (unsigned char *)authent.dat, 
		       notice->z_authent_len) == ZERR_BADFIELD) {
	    syslog (LOG_DEBUG,
		    "ZReadAscii failed (len:%s) -> AUTH_FAILED (from %s)",
		    error_message (ZERR_BADFIELD), inet_ntoa (from->sin_addr));
	    return (ZAUTH_FAILED);
	}
	authent.length = notice->z_authent_len;
	result = check_cache (notice, from);
	if (result != ZAUTH_NO)
	    return result;

	/* Well, it's not in the cache... decode it.  */
	result = krb_rd_req(&authent, SERVER_SERVICE, 
			    SERVER_INSTANCE, (int) from->sin_addr.s_addr, 
			    &dat, SERVER_SRVTAB);
	if (result == RD_AP_OK) {
	    bcopy ((void *) dat.session, (void *) a.session_key,
		   sizeof(C_Block));
	    bcopy((char *)dat.session, (char *)__Zephyr_session, 
		  sizeof(C_Block));
	    (void) sprintf(srcprincipal, "%s%s%s@%s", dat.pname, 
			   dat.pinst[0]?".":"", dat.pinst, dat.prealm);
	    if (strcmp(srcprincipal, notice->z_sender)) {
		syslog (LOG_DEBUG, "principal mismatch->AUTH_FAILED");
		return (ZAUTH_FAILED);
	    }
	    a.principal = make_zstring(srcprincipal,0);
	    a.expire_time = time (0) + 5 * 60; /* add 5 minutes */
	    a.from = *from;
	    s = (char *) xmalloc (auth_len + 1);
	    strcpy (s, notice->z_ascii_authent);
	    a.data = s;
	    a.len = auth_len;
	    add_to_cache (&a);
	    return(ZAUTH_YES);
	} else {
	    syslog (LOG_DEBUG, "krb_rd_req failed (%s)->AUTH_FAILED (from %s)",
		    krb_err_txt [result], inet_ntoa (from->sin_addr));
	    return (ZAUTH_FAILED);	/* didn't decode correctly */
	}
    }
    
    if (result = krb_get_cred(SERVER_SERVICE, SERVER_INSTANCE, 
			      __Zephyr_realm, &cred)) {
	syslog (LOG_DEBUG, "krb_get_cred failed (%s) ->AUTH_NO (from %s)",
		krb_err_txt [result], inet_ntoa (from->sin_addr));
	return (ZAUTH_NO);
    }

#ifdef NOENCRYPTION
    our_checksum = 0;
#else
    our_checksum = (ZChecksum_t)des_quad_cksum(notice->z_packet, NULL, 
					       notice->z_default_format+
					       strlen(notice->z_default_format)+1-
					       notice->z_packet, 0, cred.session);
#endif
    /* if mismatched checksum, then the packet was corrupted */
    if (our_checksum == notice->z_checksum) {
	return ZAUTH_YES;
    }
    else {
	syslog (LOG_DEBUG, "checksum mismatch->AUTH_FAILED (from %s)",
		inet_ntoa (from->sin_addr));
	return ZAUTH_FAILED;
    }
} 
#endif /* KERBEROS */
