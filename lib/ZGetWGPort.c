/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZGetWGPort function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZGetWGPort.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZGetWGPort.c,v 1.7 1988-07-26 16:41:02 jtkohl Exp $ */

#ifndef lint
static char rcsid_ZGetWGPort_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZGetWGPort.c,v 1.7 1988-07-26 16:41:02 jtkohl Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

extern char *getenv();
extern uid_t getuid();

int ZGetWGPort()
{
    char *envptr, name[128];
    FILE *fp;
    int wgport;
	
    envptr = getenv("WGFILE");
    if (!envptr) {
	(void) sprintf(name, "/tmp/wg.%d", getuid());
	envptr = name;
    } 
    if (!(fp = fopen(envptr, "r")))
	return (-1);

    /* if fscanf fails, return -1 via wgport */
    if (fscanf(fp, "%d", &wgport) != 1)
	    wgport = -1;

    (void) fclose(fp);

    return (wgport);
}