/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZOpenPort function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZOpenPort.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZOpenPort.c,v 1.9 1988-05-13 14:41:57 rfrench Exp $ */

#ifndef lint
static char rcsid_ZOpenPort_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZOpenPort.c,v 1.9 1988-05-13 14:41:57 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>
#include <sys/socket.h>

Code_t ZOpenPort(port)
    u_short *port;
{
    int retval;
    struct sockaddr_in bindin;

    (void) ZClosePort();

    if ((__Zephyr_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	__Zephyr_fd = -1;
	return (errno);
    }

    bindin.sin_family = AF_INET;

    if (port && *port)
	bindin.sin_port = *port;
    else
	bindin.sin_port = 0;

    bindin.sin_addr.s_addr = INADDR_ANY;

    if ((retval = bind(__Zephyr_fd, &bindin, sizeof(bindin))) < 0) {
	if (errno == EADDRINUSE && port && *port)
	    return (ZERR_PORTINUSE);
	else
	    return (errno);
    }

    if (!bindin.sin_port)
	if (getsockname(__Zephyr_fd, &bindin, sizeof(bindin)))
	    return (errno);
    
    __Zephyr_port = bindin.sin_port;
    __Zephyr_open = 1;

    if (port)
	*port = bindin.sin_port;

    return (ZERR_NONE);
}
