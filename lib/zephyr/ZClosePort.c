/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZClosePort function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZClosePort.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZClosePort.c,v 1.8 1995-07-07 22:10:57 ghudson Exp $ */

#include <internal.h>

#ifndef lint
static const char rcsid_ZClosePort_c[] = "$Id: ZClosePort.c,v 1.8 1995-07-07 22:10:57 ghudson Exp $";
#endif

Code_t ZClosePort()
{
    if (__Zephyr_fd >= 0 && __Zephyr_open)
	(void) close(__Zephyr_fd);

    __Zephyr_fd = -1;
    __Zephyr_open = 0;
	
    return (ZERR_NONE);
}
