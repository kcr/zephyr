/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZCompareUID function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZCmpUID.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZCmpUID.c,v 1.4 1987-07-29 15:15:22 rfrench Exp $ */

#ifndef lint
static char rcsid_ZCompareUID_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZCmpUID.c,v 1.4 1987-07-29 15:15:22 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr.h>

int ZCompareUID(uid1,uid2)
	ZUnique_Id_t *uid1,*uid2;
{
	return (!bcmp((char *)uid1,(char *)uid2,sizeof (*uid1)));
}
