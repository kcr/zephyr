/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFlushSubscriptions function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZFlsSubs.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFlsSubs.c,v 1.2 1987-07-29 15:15:37 rfrench Exp $ */

#ifndef lint
static char rcsid_ZFlushSubscriptions_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFlsSubs.c,v 1.2 1987-07-29 15:15:37 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZFlushSubscriptions()
{
	int i;
	
	if (!__subscriptions_list)
		return (ZERR_NONE);

	for (i=0;i<__subscriptions_num;i++) {
		free(__subscriptions_list[i].class);
		free(__subscriptions_list[i].classinst);
		free(__subscriptions_list[i].recipient);
	}
	
	free((char *)__subscriptions_list);

	__subscriptions_list = 0;
	__subscriptions_num = 0;

	return (ZERR_NONE);
}

