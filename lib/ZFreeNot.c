/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFreeNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZFreeNot.c,v $
 *	$Author: raeburn $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZFreeNot.c,v 1.3 1990-11-30 08:44:07 raeburn Exp $ */

#ifndef lint
static char rcsid_ZFreeNotice_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZFreeNot.c,v 1.3 1990-11-30 08:44:07 raeburn Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZFreeNotice(notice)
    ZNotice_t *notice;
{
    free(notice->z_packet);
    return 0;
}
