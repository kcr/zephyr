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
#include <sys/uio.h>

static int wait_for_hmack(ZNotice_t *, void *);

Code_t
ZSendPacket(char *packet,
	    int len,
	    int waitforack)
{
    Code_t retval;
    ZNotice_t notice, acknotice;

    if (__Zephyr_tcp_fd == -1)
        retval = Z_SendUDP(packet, len);
    else
        retval = Z_SendTCP(packet, len);

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

Code_t
Z_GetTCP(ZPacket_t *packet, int *len)
{
    unsigned short length;
    int buflen = *len;
    int ret;

    ret = read(__Zephyr_tcp_fd, &length, sizeof(length));
    if (ret < 0)
        return errno;
    if (ret < sizeof(length))
        return ZERR_INTERNAL;

    *len = ntohs(length);

    if (*len > buflen)
        return ZERR_INTERNAL;

    ret = read(__Zephyr_tcp_fd, packet, *len);
    if (ret < 0)
        return errno;
    if (ret < length)
        return ZERR_INTERNAL;

    return ZERR_NONE;
}

Code_t
Z_SendTCP(char *packet, int len)
{
    u_short length;
    int ret;
    struct iovec iov[2];
    int written = 0;

    length = htons((unsigned short) len);

    iov[0].iov_base = (void *)&length;
    iov[0].iov_len = 2;
    iov[1].iov_base = packet;
    iov[1].iov_len = len;

    ret = writev(__Zephyr_tcp_fd, iov, 2);
    if (ret < 0)
        return errno;
    if (ret < (len + 2))
        return ZERR_INTERNAL;
    return ZERR_NONE;
}
