/* This file is part of the Project Athena Zephyr Notification System.
 * It contains the version identification of the Zephyr server
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /srv/kcr/athena/zephyr/server/version.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#ifdef DEBUG
char version[] = "Zephyr Server (DEBUG) $Revision: 3.16 $";
#else
char version[] = "Zephyr Server $Revision: 3.16 $";
#endif DEBUG
#ifndef lint
#ifndef SABER
static char rcsid_version_c[] = "$Id: version.c,v 3.16 1989-11-10 09:48:59 jtkohl Exp $";
char copyright[] = "Copyright (c) 1987,1988,1989 Massachusetts Institute of Technology.\n";
#ifdef CONCURRENT
char concurrent[] = "Brain-dump concurrency enabled";
#else
char concurrent[] = "no brain-dump concurrency";
#endif CONCURRENT
#endif SABER
#endif lint
