/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZGetSender.c function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZGetSender.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZGetSender.c,v 1.2 1987-07-01 04:36:57 rfrench Exp $ */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

#include <pwd.h>

char *ZGetSender()
{
	char *tktfile;
	static char sender[128] = "";
	char pname[ANAME_SZ],pinst[INST_SZ];
	FILE *fp;
	struct passwd *pw;
	
	if (*sender)
		return (sender);

	tktfile = (char *)TKT_FILE;
	if (!(fp = fopen(tktfile,"r"))) {
		/*NOSTRICT*/
		pw = getpwuid(getuid());
		if (!pw)
			return ("unauth");
		(void) sprintf(sender,"%s@UNAUTH",pw->pw_name);
		return (sender);
	} 
	getst(fp,pname,ANAME_SZ);
	getst(fp,pinst,INST_SZ);
	(void) sprintf(sender,"%s%s%s@%s",pname,(pinst[0]?".":""),pinst,
		__Zephyr_realm);
	
	return (sender);
}

static getst(fp,s,n)
	FILE *fp;
	char *s;
	int n;
{
	int count;

	count = n;
	while (fread(s,1,1,fp) && --count)
		if (!*(s++))
			return;
	*(s++) = '\0';
	return;
}
