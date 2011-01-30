/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSetFD function.
 *
 *	Created by:	Robert French
 *
 *	$Id$
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#ifndef lint
static const char rcsid_ZSetFD_c[] = "$Id$";
#endif

#include <internal.h>

Code_t /* XXX deprecated, not in the zephyr/zephyr.h anymore */
ZSetFD(int fd)
{
    return Z_SetFD(fd);
}

Code_t
Z_SetFD(int fd)
{
	(void) ZClosePort();

	__Zephyr_fd = fd;
	__Zephyr_open = 0;
	
	return ZERR_NONE;
}

Code_t
Z_SetTCPFD(int fd)
{
    if (__Zephyr_tcp_open)
        close(__Zephyr_tcp_fd);

    __Zephyr_tcp_fd = fd;
    __Zephyr_tcp_open = 0;

    return ZERR_NONE;
}
