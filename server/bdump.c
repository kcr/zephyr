/* This file is part of the Project Athena Zephyr Notification System.
 * It contains functions for dumping server state between servers.
 *
 *	Created by:	John T. Kohl
 *
 *	$Source: /srv/kcr/athena/zephyr/server/bdump.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#ifndef lint
static char rcsid_brain_dump_c[] = "$Header: /srv/kcr/athena/zephyr/server/bdump.c,v 1.1 1987-06-19 10:30:05 jtkohl Exp $";
#endif lint

#include "zserver.h"

void
get_brain_dump(who)
struct sockaddr_in *who;
{
}

void
send_brain_dump(who)
struct sockaddr_in *who;
{
}
