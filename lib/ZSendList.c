/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSendList function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZSendList.c,v $
 *	$Author: jfc $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZSendList.c,v 1.9 1990-12-22 17:28:14 jfc Exp $ */

#ifndef lint
static char rcsid_ZSendList_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZSendList.c,v 1.9 1990-12-22 17:28:14 jfc Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZSendList(notice, list, nitems, cert_routine)
    ZNotice_t *notice;
    char *list[];
    int nitems;
    Z_AuthProc cert_routine;
{
    return(ZSrvSendList(notice, list, nitems, cert_routine, Z_XmitFragment));
}

Code_t ZSrvSendList(notice, list, nitems, cert_routine, send_routine)
    ZNotice_t *notice;
    char *list[];
    int nitems;
    int (*cert_routine)();
    int (*send_routine)();
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
    
    retval = Z_SendFragmentedNotice(&newnotice, len, send_routine);

    free(buffer);

    return (retval);
}
