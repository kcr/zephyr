/* This file is part of the Project Athena Zephyr Notification System.
 * It contains functions for dealing with acl's.
 *
 *	Created by:	John T. Kohl
 *
 *	$Id$
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>
#include "zserver.h"
#include <com_err.h>

#if !defined (lint) && !defined (SABER)
static const char rcsid_access_c[] =
    "$Id$";
#endif

/*
 *
 * External routines:
 *
 * int access_check(notice, acl, accesstype)
 *    ZNotice_t *notice;
 *    Acl *acl;
 *    Access accesstype;
 *
 * void access_init();
 *
 * void access_reinit();
 */

/*
 * Each restricted class has four ACL's associated with it,
 * governing subscriptions, transmission, and instance restrictions.
 * This module provides the 'glue' between the standard Athena ACL
 * routines and the support needed by the Zephyr server.
 */

/*
 * Our private types for the acl_types field in the Acl structure.
 * 	-TYT 8/14/90
 */
#define ACL_XMT		1
#define ACL_SUB		2
#define ACL_IWS		4
#define ACL_IUI		8

static void check_acl __P((Acl *acl));
static void check_acl_type __P((Acl *acl, Access accesstype, int typeflag));
static void access_setup __P((int first));

/*
 * check access.  return 1 if ok, 0 if not ok.
 */

int
access_check(sender, acl, accesstype)
    char *sender;
    Acl *acl;
    Access accesstype;
{
    char buf[MAXPATHLEN];	/* holds the real acl name */
    char *prefix;
    int	flag;
    int retval;

    switch (accesstype) {
      case TRANSMIT:
	prefix = "xmt";
	flag = ACL_XMT;
	break;
      case SUBSCRIBE:
	prefix = "sub";
	flag = ACL_SUB;
	break;
      case INSTWILD:
	prefix = "iws";
	flag = ACL_IWS;
	break;
      case INSTUID:
	prefix = "iui";
	flag = ACL_IUI;
	break;
      default:
	syslog(LOG_ERR, "unknown access type %d", (int) accesstype);
	return 0;
    }
    if (!(acl->acl_types & flag)) /* no acl ==> no restriction */
	return 1;
    sprintf(buf, "%s/%s-%s.acl", acl_dir, prefix, acl->acl_filename);
    /*
     * If we can't load it (because it probably doesn't exist),
     * we deny access.
     */
#if 0
    zdbug ((LOG_DEBUG, "checking %s for %s", buf, sender));
#endif
	
    retval = acl_load(buf);
    if (retval < 0) {
	syslog(LOG_DEBUG, "Error in acl_load of %s for %s", buf, sender);
	return 0;
    }
    return acl_check(buf, sender);
}

static void
check_acl(acl)
    Acl *acl;
{
    acl->acl_types = 0;
    check_acl_type(acl, TRANSMIT, ACL_XMT);
    check_acl_type(acl, SUBSCRIBE, ACL_SUB);
    check_acl_type(acl, INSTWILD, ACL_IWS);
    check_acl_type(acl, INSTUID, ACL_IUI);
}

static void
check_acl_type(acl, accesstype, typeflag)
    Acl *acl;
    Access accesstype;
    int typeflag;
{
    char 	buf[MAXPATHLEN]; /* holds the real acl name */
    char	*prefix;

    switch (accesstype) {
      case TRANSMIT:
	prefix = "xmt";
	break;
      case SUBSCRIBE:
	prefix = "sub";
	break;
      case INSTWILD:
	prefix = "iws";
	break;
      case INSTUID:
	prefix = "iui";
	break;
      default:
	syslog(LOG_ERR, "unknown access type %d", (int) accesstype);
	return;
    }
    sprintf(buf, "%s/%s-%s.acl", acl_dir, prefix, acl->acl_filename);
    if (!access(buf, F_OK))
	acl->acl_types |= typeflag;
}


/*
 * Re-init code written by TYT, 8/14/90.
 *
 * General plan of action; we reread the registry list, and add any
 * new restricted classes.  If any restricted classes disappear (this
 * should be rarely) the Acl structure is not deallocated; rather,
 * the acl_types field will be left at zero, since there will be no
 * acl files for the (non-)restricted class.
 */
static void
access_setup(first)
    int first;
{
    char buf[MAXPATHLEN];
    char class_name[512];	/* assume class names <= 511 bytes */
    FILE *registry;
    Acl *acl;
    int len;
    char *colon_idx;
    Code_t retval = 0;

    sprintf(buf, "%s/%s", acl_dir, ZEPHYR_CLASS_REGISTRY);
    registry = fopen(buf, "r");
    if (!registry) {
	syslog(LOG_ERR, "no registry available, all classes are free");
	return;
    }
    while (fgets(class_name, 512, registry)) {
	colon_idx = strchr(class_name, ':');
	if (colon_idx != NULL)
	    *colon_idx = '\0';
	else if ((len = strlen(class_name)) != 0)
	    class_name[len - 1] = '\0';
	acl = 0;
	if (!first) {
	    String *z;

	    z = make_string(class_name,1);
	    acl = class_get_acl(z);
	    free_string(z);
	}
	if (!acl) {
	    acl = (Acl *) malloc(sizeof(Acl));
	    if (!acl) {
		syslog(LOG_ERR, "no mem acl alloc");
		abort();
	    }
	    acl->acl_filename = strsave(class_name);
	    check_acl(acl);
		    
	    if (!first) {
		/* Try to restrict already existing class */
		retval = class_restrict(class_name, acl);
		if (retval == ZSRV_NOCLASS)
		    retval = class_setup_restricted(class_name, acl);
	    } else {
		retval = class_setup_restricted(class_name, acl);
	    }
	}
	if (retval) {
	    syslog(LOG_ERR, "can't restrict %s: %s",
		   class_name, error_message(retval));
	    continue;
	}
	zdbug((LOG_DEBUG, "restricted %s", class_name));
    }
    fclose(registry);
}

void
access_init()
{
    access_setup(1);
}

void
access_reinit()
{
    acl_cache_reset();
    access_setup(0);
}
