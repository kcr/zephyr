/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFlushSubscriptions function.
 *
 *	Created by:	Robert French
 *
 *	$Id$
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <internal.h>

#ifndef lint
static const char rcsid_ZFlushSubscriptions_c[] = "$Id$";
#endif

Code_t
ZFlushSubscriptions(void)
{
	register int i;
	
	if (!__subscriptions_list)
		return (ZERR_NONE);

	for (i=0;i<__subscriptions_num;i++) {
	    /* XXX we know we can free these because Z_RetSubs allocates them */
	    free((char *)__subscriptions_list[i].zsub_class);
	    free((char *)__subscriptions_list[i].zsub_classinst);
	    free((char *)__subscriptions_list[i].zsub_recipient);
	}
	
	free((char *)__subscriptions_list);

	__subscriptions_list = 0;
	__subscriptions_num = 0;

	return (ZERR_NONE);
}

