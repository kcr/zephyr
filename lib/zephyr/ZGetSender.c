/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZGetSender.c function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZGetSender.c,v $
 *	$Author: raeburn $
 *
 *	Copyright (c) 1987, 1991 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZGetSender.c,v 1.8 1991-03-21 11:41:23 raeburn Exp $ */

#ifndef lint
static char rcsid_ZGetSender_c[] =
    "$Zephyr: ZGetSender.c,v 1.7 88/05/13 15:05:07 rfrench Exp $";
#endif

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

#include <pwd.h>

uid_t getuid();

char *ZGetSender()
{
    struct passwd *pw;
#ifdef KERBEROS
    char pname[ANAME_SZ], pinst[INST_SZ];
    static char sender[ANAME_SZ+INST_SZ+REALM_SZ+3] = "";
#else
    static char sender[128] = "";
#endif

    /* Return it if already cached */
    if (*sender)
	return (sender);

#ifdef KERBEROS
    if (tf_init((char *)TKT_FILE, R_TKT_FIL) == KSUCCESS) {
	if ((tf_get_pname(pname) == KSUCCESS) &&
	    (tf_get_pinst(pinst) == KSUCCESS)) {
	    (void) sprintf(sender, "%s%s%s@%s", pname, (pinst[0]?".":""),
			   pinst, ZGetRealm ());
	    tf_close();
	    return (sender);
	}
	tf_close();
    }
#endif

    /* XXX a uid_t is a u_short (now),  but getpwuid
     * wants an int. AARGH! */
    pw = getpwuid((int) getuid());
    if (!pw)
	return ("unknown");
    (void) sprintf(sender, "%s@%s", pw->pw_name, ZGetRealm ());
    return (sender);
}