/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZCompareUIDPred function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/ZCmpUIDP.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/ZCmpUIDP.c,v 1.6 1997-09-14 21:52:29 ghudson Exp $ */

#ifndef lint
static char rcsid_ZCompareUIDPred_c[] = "$Header: /srv/kcr/athena/zephyr/lib/ZCmpUIDP.c,v 1.6 1997-09-14 21:52:29 ghudson Exp $";
#endif

#include <internal.h>

int ZCompareUIDPred(notice, uid)
    ZNotice_t *notice;
    void *uid;
{
    return (ZCompareUID(&notice->z_uid, (ZUnique_Id_t *) uid));
}

int ZCompareMultiUIDPred(notice, uid)
    ZNotice_t *notice;
    void *uid;
{
    return (ZCompareUID(&notice->z_multiuid, (ZUnique_Id_t *) uid));
}
