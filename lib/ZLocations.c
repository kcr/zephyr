/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSetLocation function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZLocations.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZLocations.c,v 1.7 1987-07-01 04:38:16 rfrench Exp $ */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

#include <pwd.h>
#include <sys/file.h>

Code_t ZSetLocation()
{
	char bfr[BUFSIZ];
	int quiet;
	struct passwd *pw;
	
        quiet = 0;
	/*NOSTRICT*/
	if (pw = getpwuid(getuid())) {
		(void) sprintf(bfr,"%s/.hideme",pw->pw_dir);
		quiet = !access(bfr,F_OK);
	} 

	return (Z_SendLocation(LOGIN_CLASS,quiet?LOGIN_QUIET_LOGIN:
			       LOGIN_USER_LOGIN));
}
