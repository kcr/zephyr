/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZGetWGPort function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZGetWGPort.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZGetWGPort.c,v 1.6 1988-06-24 10:28:10 jtkohl Exp $ */

#ifndef lint
static char rcsid_ZGetWGPort_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZGetWGPort.c,v 1.6 1988-06-24 10:28:10 jtkohl Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

extern char *getenv();
extern uid_t getuid();

short ZGetWGPort()
{
    char *envptr, name[128];
    FILE *fp;
    short wgport;
	
    envptr = getenv("WGFILE");
    if (!envptr) {
	(void) sprintf(name, "/tmp/wg.%d", getuid());
	envptr = name;
    } 
    if (!(fp = fopen(envptr, "r")))
	return (-1);

    /* if fscanf fails, return -1 via wgport */
    if (fscanf(fp, "%hd", &wgport) != 1)
	    wgport = -1;

    (void) fclose(fp);

    return (wgport);
}
