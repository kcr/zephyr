/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSetDestAddr function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZSetDest.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZSetDest.c,v 1.4 1995-06-30 22:04:44 ghudson Exp $ */

#ifndef lint
static char rcsid_ZSetDestAddr_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZSetDest.c,v 1.4 1995-06-30 22:04:44 ghudson Exp $";
#endif

#include <internal.h>

Code_t ZSetDestAddr(addr)
	struct	sockaddr_in *addr;
{
	__HM_addr = *addr;

	__HM_set = 1;
	
	return (ZERR_NONE);
}
