/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZGetSubscriptions function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZGetSubs.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZGetSubs.c,v 1.1 1987-07-09 20:02:49 rfrench Exp $ */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

#define min(a,b) ((a)<(b)?(a):(b))
	
Code_t ZGetSubscriptions(subscription,numsubs)
	ZSubscription_t *subscription;
	int *numsubs;
{
	int i;
	
	if (!__subscriptions_list)
		return (ZERR_NOSUBSCRIPTIONS);

	if (__subscriptions_next == __subscriptions_num)
		return (ZERR_NOMORESUBSCRIPTIONS);
	
	for (i=0;i<min(*numsubs,__subscriptions_num-__subscriptions_next);i++)
		subscription[i] = __subscriptions_list[i+__subscriptions_next];

	if (__subscriptions_num-__subscriptions_next < *numsubs)
		*numsubs = __subscriptions_num-__subscriptions_next;

	__subscriptions_next += *numsubs;
	
	return (ZERR_NONE);
}
