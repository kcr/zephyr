/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZClosePort function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZClosePort.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZClosePort.c,v 1.4 1987-07-29 15:15:17 rfrench Exp $ */

#ifndef lint
static char rcsid_ZClosePort_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZClosePort.c,v 1.4 1987-07-29 15:15:17 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZClosePort()
{
	if (__Zephyr_fd >= 0 && __Zephyr_open)
		(void) close(__Zephyr_fd);

	__Zephyr_fd = -1;
	__Zephyr_open = 0;
	
	return (ZERR_NONE);
}
