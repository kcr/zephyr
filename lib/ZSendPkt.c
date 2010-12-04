/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZSendPacket function.
 *
 *	Created by:	Robert French
 *
 *	$Id$
 *
 *	Copyright (c) 1987,1991 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#ifndef lint
static const char rcsid_ZSendPacket_c[] =
    "$Id$";
#endif

#include <internal.h>
#include <sys/socket.h>

static int wait_for_hmack(ZNotice_t *, void *);

Code_t
ZSendPacket(char *packet,
	    int len,
	    int waitforack)
{
    Code_t retval;
    ZNotice_t notice, acknotice;

    retval = Z_SendUDP(packet, len);
    if (retval != ZERR_NONE)
        return ZERR_NONE;

    if (!waitforack)
	return ZERR_NONE;

    retval = ZParseNotice(packet, len, &notice);
    if (retval != ZERR_NONE)
	return retval;

    retval = Z_WaitForNotice(&acknotice, wait_for_hmack, &notice.z_uid,
			     HM_TIMEOUT);
    if (retval == ETIMEDOUT)
        return ZERR_HMDEAD;
    if (retval == ZERR_NONE)
        ZFreeNotice (&acknotice);

    return retval;
}

Code_t
Z_SendUDP(char *packet, int len) {
    struct sockaddr_in dest;
    Code_t retval = ZERR_NONE;

    if (!packet || len < 0)
	return ZERR_ILLVAL;

    if (len > Z_MAXPKTLEN)
	return ZERR_PKTLEN;

    if (ZGetFD() < 0)
        retval = ZOpenPort((u_short *)0);
    if (retval != ZERR_NONE)
        return (retval);

    dest = ZGetDestAddr();

    if (sendto(ZGetFD(), packet, len, 0, (struct sockaddr *)&dest,
	       sizeof(dest)) < 0)
	return errno;

    return ZERR_NONE;
}

static int
wait_for_hmack(ZNotice_t *notice,
	       void *uid)
{
    return (notice->z_kind == HMACK && ZCompareUID(&notice->z_uid, (ZUnique_Id_t *)uid));
}
