/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZFreeNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZFreeNot.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZFreeNot.c,v 1.5 1997-09-14 21:52:36 ghudson Exp $ */

#ifndef lint
static char rcsid_ZFreeNotice_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZFreeNot.c,v 1.5 1997-09-14 21:52:36 ghudson Exp $";
#endif

#include <internal.h>

Code_t ZFreeNotice(notice)
    ZNotice_t *notice;
{
    free(notice->z_packet);
    return 0;
}
