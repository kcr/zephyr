/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSetServerState function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZSetSrv.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZSetSrv.c,v 1.1 1987-06-24 04:24:51 rfrench Exp $ */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZSetServerState(state)
	int	state;
{
	__Zephyr_server = state;
	
	return (ZERR_NONE);
}
