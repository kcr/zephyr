/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZCompareUID function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZCmpUID.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZCmpUID.c,v 1.9 1995-06-30 22:03:56 ghudson Exp $ */

#ifndef lint
static char rcsid_ZCompareUID_c[] = "$Id: ZCmpUID.c,v 1.9 1995-06-30 22:03:56 ghudson Exp $";
#endif

#include <internal.h>

int ZCompareUID(uid1, uid2)
    ZUnique_Id_t *uid1, *uid2;
{
    return (!memcmp((char *)uid1, (char *)uid2, sizeof (*uid1)));
}
