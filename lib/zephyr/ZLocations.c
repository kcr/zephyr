/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSetLocation, ZUnsetLocation, and
 * ZFlushMyLocations functions.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZLocations.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZLocations.c,v 1.19 1988-06-23 10:31:37 jtkohl Exp $ */

#ifndef lint
static char rcsid_ZLocations_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZLocations.c,v 1.19 1988-06-23 10:31:37 jtkohl Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

#include <pwd.h>
#include <sys/file.h>
#include <sys/param.h>
#include <netdb.h>

extern char *getenv();

Code_t ZSetLocation(exposure)
    char *exposure;
{
    return (Z_SendLocation(LOGIN_CLASS, exposure, ZAUTH, 
			   "$sender logged in to $1 on $3 at $2"));
}

Code_t ZUnsetLocation()
{
    return (Z_SendLocation(LOGIN_CLASS, LOGIN_USER_LOGOUT, ZNOAUTH, 
			   "$sender logged out of $1 on $3 at $2"));
}

Code_t ZFlushMyLocations()
{
    return (Z_SendLocation(LOGIN_CLASS, LOGIN_USER_FLUSH, ZAUTH, ""));
}

Z_SendLocation(class, opcode, auth, format)
    char *class;
    char *opcode;
    int (*auth)();
    char *format;
{
    char *ttyname(), *ctime();

    int retval;
    long ourtime;
    ZNotice_t notice, retnotice;
    char *bptr[3], host[MAXHOSTNAMELEN], mytty[MAXHOSTNAMELEN];
    char *display;
    struct hostent *hent;

    notice.z_kind = ACKED;
    notice.z_port = 0;
    notice.z_class = class;
    notice.z_class_inst = ZGetSender();
    notice.z_opcode = opcode;
    notice.z_sender = 0;
    notice.z_recipient = "";
    notice.z_num_other_fields = 0;
    notice.z_default_format = format;

    if (gethostname(host, MAXHOSTNAMELEN) < 0)
	return (errno);

    hent = gethostbyname(host);
    if (!hent)
	bptr[0] = "unknown";
    else {
	(void) strcpy(host, hent->h_name);
	bptr[0] = host;
    } 

    ourtime = time((long *)0);
    bptr[1] = ctime(&ourtime);
    bptr[1][strlen(bptr[1])-1] = '\0';

    if ((display = getenv("DISPLAY")) && *display) {
	(void) strcpy(mytty, display);
	bptr[2] = mytty;
    }
    else {
	(void) strcpy(mytty, ttyname(0));
	bptr[2] = rindex(mytty, '/');
	if (bptr[2])
	    bptr[2]++;
	else
	    bptr[2] = mytty;
    }
	
    if ((retval = ZSendList(&notice, bptr, 3, auth)) != ZERR_NONE)
	return (retval);

    if ((retval = ZIfNotice(&retnotice, (struct sockaddr_in *)0, 
			    ZCompareUIDPred, (char *)&notice.z_uid)) !=
	ZERR_NONE)
	return (retval);

    if (retnotice.z_kind == SERVNAK) {
	if (!retnotice.z_message_len) {
	    ZFreeNotice(&retnotice);
	    return (ZERR_SERVNAK);
	}
	if (!strcmp(retnotice.z_message, ZSRVACK_NOTSENT)) {
	    ZFreeNotice(&retnotice);
	    return (ZERR_AUTHFAIL);
	}
	if (!strcmp(retnotice.z_message, ZSRVACK_FAIL)) {
	    ZFreeNotice(&retnotice);
	    return (ZERR_LOGINFAIL);
	}
	ZFreeNotice(&retnotice);
	return (ZERR_SERVNAK);
    } 
	
    if (retnotice.z_kind != SERVACK) {
	ZFreeNotice(&retnotice);
	return (ZERR_INTERNAL);
    }

    if (!retnotice.z_message_len) {
	ZFreeNotice(&retnotice);
	return (ZERR_INTERNAL);
    }

    if (strcmp(retnotice.z_message, ZSRVACK_SENT) &&
	strcmp(retnotice.z_message, ZSRVACK_NOTSENT)) {
	ZFreeNotice(&retnotice);
	return (ZERR_INTERNAL);
    }

    ZFreeNotice(&retnotice);
	
    return (ZERR_NONE);
}
