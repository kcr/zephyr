/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSetLocation, ZUnsetLocation, and
 * ZFlushMyLocations functions.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZLocations.c,v $
 *	$Author: raeburn $
 *
 *	Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZLocations.c,v 1.27 1990-11-16 11:01:33 raeburn Exp $ */

#ifndef lint
static char rcsid_ZLocations_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZLocations.c,v 1.27 1990-11-16 11:01:33 raeburn Exp $";
#endif lint

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
    register int i;
    int zfd;
    fd_set fdmask;
    struct timeval tv, t0;

    (void) bzero((char *)&notice, sizeof(notice));
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
	    if (!hent)
		    (void) strcpy(host, "unknown");
	    else
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
			bptr[2] = rindex(ttyp, '/');
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

    tv.tv_sec = HM_TIMEOUT;
    tv.tv_usec = 0;
    FD_ZERO (&fdmask);
    zfd = ZGetFD();
    FD_SET (zfd, &fdmask);
    gettimeofday (&t0, 0);
    t0.tv_sec += HM_TIMEOUT;
    while (1) {
	i = select (zfd + 1, &fdmask, (fd_set *) 0, (fd_set *) 0, &tv);
	if (i > 0) {
	    retval = ZCheckIfNotice (&retnotice, (struct sockaddr_in*) 0,
				     ZCompareUIDPred, (char *)&notice.z_uid);
	    if (retval == ZERR_NONE)
		break;
	    if (retval != ZERR_NONOTICE)
		return retval;
	} else if (i == 0)
	    return ETIMEDOUT;	/* timeout */
	else if (errno != EINTR)
	    return errno;
	gettimeofday (&tv, 0);
	tv.tv_usec = t0.tv_usec - tv.tv_usec;
	if (tv.tv_usec < 0) {
	    tv.tv_usec += 1000000;
	    tv.tv_sec = t0.tv_sec - tv.tv_sec - 1;
	} else {
	    tv.tv_sec = t0.tv_sec - tv.tv_sec;
	}
    }

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
