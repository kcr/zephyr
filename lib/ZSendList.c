/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSendList function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZSendList.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZSendList.c,v 1.7 1988-05-17 21:23:37 rfrench Exp $ */

#ifndef lint
static char rcsid_ZSendList_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZSendList.c,v 1.7 1988-05-17 21:23:37 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZSendList(notice, list, nitems, cert_routine)
    ZNotice_t *notice;
    char *list[];
    int nitems;
    int (*cert_routine)();
{
    Code_t retval;
    ZNotice_t newnotice;
    char *buffer;
    int len;

    if ((retval = ZFormatNoticeList(notice, list, nitems, &buffer, 
				    &len, cert_routine)) != ZERR_NONE)
	return (retval);

    if ((retval = ZParseNotice(buffer, len, &newnotice)) != ZERR_NONE)
	return (retval);
    
    retval = Z_SendFragmentedNotice(&newnotice, len);

    free(buffer);

    return (retval);
}
