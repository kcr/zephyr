/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZNewLocateUser function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZNewLocU.c,v $
 *	$Author: lwvanels $
 *
 *	Copyright (c) 1987,1988,1991 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZNewLocU.c,v 1.8 1992-08-10 19:13:15 lwvanels Exp $ */

#ifndef lint
static char rcsid_ZNewLocateUser_c[] =
    "$Id: ZNewLocU.c,v 1.8 1992-08-10 19:13:15 lwvanels Exp $";
#endif

#include <zephyr/zephyr_internal.h>

Code_t ZLocateUser(user, nlocs, auth)
    char *user;
    int *nlocs;
    Z_AuthProc auth;
{
    register int retval;
    ZNotice_t notice;
    ZAsyncLocateData_t zald;

    (void) ZFlushLocations();	/* ZFlushLocations never fails (the library
				   is allowed to know this). */

    if ((retval = ZRequestLocations(user, &zald, UNACKED, auth)) != ZERR_NONE)
	return(retval);

    retval = Z_WaitForNotice (&notice, ZCompareALDPred,
			      (char *) &zald, SRV_TIMEOUT);
    if (retval == ZERR_NONOTICE)
	return ETIMEDOUT;
    if (retval != ZERR_NONE)
	return retval;

    if ((retval = ZParseLocations(&notice, &zald, nlocs, NULL)) != ZERR_NONE) {
	ZFreeNotice(&notice);
	return(retval);
    }

    ZFreeNotice(&notice);
    ZFreeALD(&zald);
    return(ZERR_NONE);
}
