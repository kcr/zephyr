/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSetServerState function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZSetSrv.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZSetSrv.c,v 1.4 1995-06-30 22:04:48 ghudson Exp $ */

#ifndef lint
static char rcsid_ZSetServerState_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZSetSrv.c,v 1.4 1995-06-30 22:04:48 ghudson Exp $";
#endif

#include <internal.h>

Code_t ZSetServerState(state)
	int	state;
{
	__Zephyr_server = state;
	
	return (ZERR_NONE);
}
