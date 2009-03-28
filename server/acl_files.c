/* This file is part of the Project Athena Zephyr Notification System.
 * It contains functions for maintaining Access Control Lists.
 *
 *	Created by:	John T. Kohl
 *
 *	$Id$
 *
 *	Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

/* Define this if you really want the ACL-writing code included.  */

/*
 * Stolen from lib/acl_files.c because acl_load needs to be externally
 * declared and not statically declared.
 */

#include <zephyr/mit-copyright.h>
#include "zserver.h"
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/* this needs to be rethought for a world without krb4 */

#ifndef SABER
#ifndef lint
static const char rcsid_acl_files_c[] = "$Id$";
#endif /* lint */
#endif /* SABER */

/*** Routines for manipulating access control list files ***/

#ifndef ANAME_SZ
#define ANAME_SZ 40
#define INST_SZ 40
#endif /* XXX */

/* "aname.inst@realm" */
#ifndef MAX_PRINCIPAL_SIZE
#define MAX_PRINCIPAL_SIZE  (ANAME_SZ + INST_SZ + REALM_SZ + 3)
#endif
#define INST_SEP '.'
#define REALM_SEP '@'

#define LINESIZE 2048		/* Maximum line length in an acl file */

#define NEW_FILE "%s.~NEWACL~"	/* Format for name of altered acl file */
#define WAIT_TIME 300		/* Maximum time allowed write acl file */

#define CACHED_ACLS 64		/* How many acls to cache */
#define ACL_LEN 256		/* Twice a reasonable acl length */

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#define COR(a,b) ((a!=NULL)?(a):(b))

/* Canonicalize a principal name */
/* If instance is missing, it becomes "" */
/* If realm is missing, it becomes the local realm */
/* Canonicalized form is put in canon, which must be big enough to hold
   MAX_PRINCIPAL_SIZE characters */
void acl_canonicalize_principal(char *principal,
				char *canon)
{
    char *end;
    char *dot, *atsign;
    int len;

    dot = strchr(principal, INST_SEP);
    atsign = strchr(principal, REALM_SEP);

    /* Maybe we're done already */
    if (dot != NULL && atsign != NULL) {
	if (dot < atsign) {
	    /* It's for real */
	    /* Copy into canon */
	    strncpy(canon, principal, MAX_PRINCIPAL_SIZE);
	    canon[MAX_PRINCIPAL_SIZE-1] = '\0';
	    return;
	} else {
	    /* Nope, it's part of the realm */
	    dot = NULL;
	}
    }
    
    /* No such luck */
    end = principal + strlen(principal);

    /* Get the principal name */
    len = MIN(ANAME_SZ, COR(dot, COR(atsign, end)) - principal);
    strncpy(canon, principal, len);
    canon += len;

    /* Add INST_SEP */
    *canon++ = INST_SEP;

    /* Get the instance, if it exists */
    if (dot != NULL) {
	++dot;
	len = MIN(INST_SZ, COR(atsign, end) - dot);
	strncpy(canon, dot, len);
	canon += len;
    }

    /* Add REALM_SEP */
    *canon++ = REALM_SEP;

    /* Get the realm, if it exists */
    /* Otherwise, default to local realm */
    if (atsign != NULL) {
	++atsign;
	len = MIN(REALM_SZ, end - atsign);
	strncpy(canon, atsign, len);
	canon += len;
	*canon++ = '\0';
    } 
#ifdef HAVE_KRB4
    else if (krb_get_lrealm(canon, 1) != KSUCCESS) {
	strcpy(canon, KRB_REALM);
    }
#endif
}

/* Eliminate all whitespace character in buf */
/* Modifies its argument */
static void
nuke_whitespace(char *buf)
{
    char *pin, *pout;

    for (pin = pout = buf; *pin != '\0'; pin++)
	if (!isspace(*pin)) *pout++ = *pin;
    *pout = '\0';		/* Terminate the string */
}

/* Hash table stuff */

struct hashtbl {
    int size;			/* Max number of entries */
    int entries;		/* Actual number of entries */
    char **tbl;			/* Pointer to start of table */
};

/* Make an empty hash table of size s */
static struct hashtbl *
make_hash(int size)
{
    struct hashtbl *h;

    if (size < 1) size = 1;
    h = (struct hashtbl *) malloc(sizeof(struct hashtbl));
    h->size = size;
    h->entries = 0;
    h->tbl = (char **) calloc(size, sizeof(char *));
    return(h);
}

/* Destroy a hash table */
static void
destroy_hash(struct hashtbl *h)
{
    int i;

    for (i = 0; i < h->size; i++) {
	if (h->tbl[i] != NULL) free(h->tbl[i]);
    }
    free(h->tbl);
    free(h);
}

/* Compute hash value for a string */
static unsigned int
hashval(char *s)
{
    unsigned hv;

    for (hv = 0; *s != '\0'; s++) {
	hv ^= ((hv << 3) ^ *s);
    }
    return(hv);
}

/* Add an element to a hash table */
static void
add_hash(struct hashtbl *h,
	 char *el)
{
    unsigned hv;
    char *s;
    char **old;
    int i;

    /* Make space if it isn't there already */
    if (h->entries + 1 > (h->size >> 1)) {
	old = h->tbl;
	h->tbl = (char **) calloc(h->size << 1, sizeof(char *));
	for (i = 0; i < h->size; i++) {
	    if (old[i] != NULL) {
		hv = hashval(old[i]) % (h->size << 1);
		while(h->tbl[hv] != NULL) hv = (hv+1) % (h->size << 1);
		h->tbl[hv] = old[i];
	    }
	}
	h->size = h->size << 1;
	free(old);
    }

    hv = hashval(el) % h->size;
    while(h->tbl[hv] != NULL && strcmp(h->tbl[hv], el)) hv = (hv+1) % h->size;
    s = (char *) malloc(strlen(el)+1);
    strcpy(s, el);
    h->tbl[hv] = s;
    h->entries++;
}

/* Returns nonzero if el is in h */
static int
check_hash(struct hashtbl *h,
	   char *el)
{
    unsigned hv;

    for (hv = hashval(el) % h->size; h->tbl[hv]; hv = (hv + 1) % h->size) {
	if (!strcmp(h->tbl[hv], el)) {
	    return 1;
	}
    }
    return 0;
}

struct acl {
    char filename[LINESIZE];	/* Name of acl file */
    struct hashtbl *acl;	/* Acl entries */
};

static struct acl acl_cache[CACHED_ACLS];

static int acl_cache_count = 0;
static int acl_cache_next = 0;

/* Returns < 0 if unsuccessful in loading acl */
/* Returns index into acl_cache otherwise */
/* Note that if acl is already loaded, this is just a lookup */
int acl_load(char *name)
{
    int i;
    FILE *f;
    char buf[MAX_PRINCIPAL_SIZE];
    char canon[MAX_PRINCIPAL_SIZE];

    /* See if it's there already */
    for (i = 0; i < acl_cache_count; i++) {
	if (!strcmp(acl_cache[i].filename, name))
	    goto got_it;
    }

    /* It isn't, load it in */
    /* maybe there's still room */
    if (acl_cache_count < CACHED_ACLS) {
	i = acl_cache_count++;
    } else {
	/* No room, clean one out */
	i = acl_cache_next;
	acl_cache_next = (acl_cache_next + 1) % CACHED_ACLS;
	if (acl_cache[i].acl) {
	    destroy_hash(acl_cache[i].acl);
	    acl_cache[i].acl = (struct hashtbl *) 0;
	}
    }

    /* Set up the acl */
    strcpy(acl_cache[i].filename, name);
    /* Force reload */
    acl_cache[i].acl = (struct hashtbl *) 0;

  got_it:
    /*
     * See if we need to reload the ACL
     */
    if (acl_cache[i].acl == (struct hashtbl *) 0) {
	/* Gotta reload */
	if ((f = fopen(name, "r")) == NULL) {
	    syslog(LOG_ERR, "Error loading acl file %s: %m", name);
	    return -1;
	}
	if (acl_cache[i].acl) destroy_hash(acl_cache[i].acl);
	acl_cache[i].acl = make_hash(ACL_LEN);
	while(fgets(buf, sizeof(buf), f) != NULL) {
	    nuke_whitespace(buf);
	    acl_canonicalize_principal(buf, canon);
	    add_hash(acl_cache[i].acl, canon);
	}
	fclose(f);
    }
    return(i);
}

/*
 * This destroys all cached ACL's so that new ones will be loaded in
 * the next time they are requested.
 */
void
acl_cache_reset(void)
{
	int	i;
	
	/* See if it's there already */
	for (i = 0; i < acl_cache_count; i++)
	    if (acl_cache[i].acl) {
		destroy_hash(acl_cache[i].acl);
		acl_cache[i].acl = (struct hashtbl *) 0;
	    }
	acl_cache_count = 0;
	acl_cache_next = 0;
    }


/* Returns nonzero if it can be determined that acl contains principal */
/* Principal is not canonicalized, and no wildcarding is done */
int
acl_exact_match(char *acl,
		char *principal)
{
    int idx;

    return((idx = acl_load(acl)) >= 0
	   && check_hash(acl_cache[idx].acl, principal));
}

/* Returns nonzero if it can be determined that acl contains principal */
/* Recognizes wildcards in acl. */
int
acl_check(char *acl,
	  char *principal)
{
    char buf[MAX_PRINCIPAL_SIZE];
    char canon[MAX_PRINCIPAL_SIZE];
    char *instance, *realm;
    int p, i, r;

    /* Parse into principal, instance, and realm. */
    acl_canonicalize_principal(principal, canon);
    instance = (char *) strchr(canon, INST_SEP);
    *instance++ = 0;
    realm = (char *) strchr(instance, REALM_SEP);
    *realm++ = 0;

    for (p = 0; p <= 1; p++) {
	for (i = 0; i <= 1; i++) {
	    for (r = 0; r <= 1; r++) {
		sprintf(buf, "%s%c%s%c%s", (p) ? canon : "*", INST_SEP,
			(i) ? instance : "*", REALM_SEP, (r) ? realm : "*");
		if (acl_exact_match(acl, buf))
		    return 1;
	    }
	}
    }
       
    return(0);
}
