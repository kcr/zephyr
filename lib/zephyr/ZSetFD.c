/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSetFD function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZSetFD.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZSetFD.c,v 1.7 1995-06-30 22:04:46 ghudson Exp $ */

#ifndef lint
static char rcsid_ZSetFD_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZSetFD.c,v 1.7 1995-06-30 22:04:46 ghudson Exp $";
#endif

#include <internal.h>

Code_t ZSetFD(fd)
	int	fd;
{
	(void) ZClosePort();

	__Zephyr_fd = fd;
	__Zephyr_open = 0;
	
	return (ZERR_NONE);
}
