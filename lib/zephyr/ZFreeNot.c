/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFreeNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZFreeNot.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFreeNot.c,v 1.5 1995-06-30 22:04:10 ghudson Exp $ */

#ifndef lint
static char rcsid_ZFreeNotice_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZFreeNot.c,v 1.5 1995-06-30 22:04:10 ghudson Exp $";
#endif

#include <internal.h>

Code_t ZFreeNotice(notice)
    ZNotice_t *notice;
{
    free(notice->z_packet);
    return 0;
}
