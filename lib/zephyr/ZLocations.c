/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSetLocation, ZUnsetLocation, and
 * ZFlushMyLocations functions.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZLocations.c,v $
 *	$Author: probe $
 *
 *	Copyright (c) 1987,1988,1991 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZLocations.c,v 1.33 1993-11-19 15:23:03 probe Exp $ */

#ifndef lint
static char rcsid_ZLocations_c[] =
    "$Zephyr: /afs/athena.mit.edu/astaff/project/zephyr/src/lib/RCS/ZLocations.c,v 1.30 90/12/20 03:04:39 raeburn Exp $";
#endif

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

#include <pwd.h>
#include <sys/file.h>
#include <sys/param.h>
#include <netdb.h>

extern char *getenv();
extern int errno;

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

static char host[MAXHOSTNAMELEN], mytty[MAXPATHLEN];
static int reenter = 0;

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
    char *bptr[3];
#ifdef X11
    char *display;
#endif /* X11 */
    char *ttyp;
    struct hostent *hent;
    short wg_port = ZGetWGPort();

    (void) memset((char *)&notice, 0, sizeof(notice));
    notice.z_kind = ACKED;
    notice.z_port = (u_short) ((wg_port == -1) ? 0 : wg_port);
    notice.z_class = class;
    notice.z_class_inst = ZGetSender();
    notice.z_opcode = opcode;
    notice.z_sender = 0;
    notice.z_recipient = "";
    notice.z_num_other_fields = 0;
    notice.z_default_format = format;

    /*
      keep track of what we said before so that we can be consistent
      when changing location information.
      This is done mainly for the sake of the WindowGram client.
     */

    if (!reenter) {
	    if (gethostname(host, MAXHOSTNAMELEN) < 0)
		    return (errno);

	    hent = gethostbyname(host);
	    if (hent)
		    (void) strcpy(host, hent->h_name);
	    bptr[0] = host;
#ifdef X11
	    if ((display = getenv("DISPLAY")) && *display) {
		    (void) strcpy(mytty, display);
		    bptr[2] = mytty;
	    } else {
#endif /* X11 */
		    ttyp = ttyname(0);
		    if (ttyp) {
			bptr[2] = strrchr(ttyp, '/');
			if (bptr[2])
			    bptr[2]++;
			else
			    bptr[2] = ttyp;
		    }
		    else
			bptr[2] = "unknown";
		    (void) strcpy(mytty, bptr[2]);
#ifdef X11
	    }
#endif /* X11 */
	    reenter = 1;
    } else {
	    bptr[0] = host;
	    bptr[2] = mytty;
    }

    ourtime = time((long *)0);
    bptr[1] = ctime(&ourtime);
    bptr[1][strlen(bptr[1])-1] = '\0';

	
    if ((retval = ZSendList(&notice, bptr, 3, auth)) != ZERR_NONE)
	return (retval);

    retval = Z_WaitForNotice (&retnotice, ZCompareUIDPred, &notice.z_uid,
			      SRV_TIMEOUT);
    if (retval != ZERR_NONE)
      return retval;

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
