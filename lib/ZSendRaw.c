/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSendRawNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZSendRaw.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZSendRaw.c,v 1.3 1988-05-17 21:23:51 rfrench Exp $ */

#ifndef lint
static char rcsid_ZSendRawNotice_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZSendRaw.c,v 1.3 1988-05-17 21:23:51 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZSendRawNotice(notice)
    ZNotice_t *notice;
{
    Code_t retval;
    ZNotice_t newnotice;
    char *buffer;
    int len;

    if ((retval = ZFormatRawNotice(notice, &buffer, &len)) !=
	ZERR_NONE)
	return (retval);

    if ((retval = ZParseNotice(buffer, len, &newnotice)) != ZERR_NONE)
	return (retval);
    
    retval = Z_SendFragmentedNotice(&newnotice);

    free(buffer);

    return (retval);
}
