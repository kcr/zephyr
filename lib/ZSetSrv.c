/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSetServerState function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZSetSrv.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZSetSrv.c,v 1.4 1997-09-14 21:53:01 ghudson Exp $ */

#ifndef lint
static char rcsid_ZSetServerState_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZSetSrv.c,v 1.4 1997-09-14 21:53:01 ghudson Exp $";
#endif

#include <internal.h>

Code_t ZSetServerState(state)
	int	state;
{
	__Zephyr_server = state;
	
	return (ZERR_NONE);
}
