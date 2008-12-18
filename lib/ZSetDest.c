/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSetDestAddr function.
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
static char rcsid_ZSetDestAddr_c[] = "$Id$";
#endif

#include <internal.h>

Code_t
ZSetDestAddr(struct sockaddr_in *addr)
{
	__HM_addr = *addr;

	__HM_set = 1;
	
	return (ZERR_NONE);
}
