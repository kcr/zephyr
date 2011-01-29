/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZReadZcode function.
 *
 *	Created by:	Jeffrey Hutzelman
 *
 *	$Id$
 *
 *	Copyright (c) 1987, 1990, 2002 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#ifndef lint
static const char rcsid_ZReadZcode_c[] = "$Id$";
#endif /* lint */

#include <internal.h>
#include <assert.h>


Code_t
ZReadZcode(const unsigned char *ptr,
	   unsigned char *field,
	   int max,
	   int *len)
{
    int n = 0;

    if (*ptr++ != 'Z')
        return ZERR_BADFIELD;

    while (*ptr && n < max) {
        if (*ptr == 0xff) {
            ptr++;
            switch (*ptr++) {
                case 0xf0: field[n++] = 0x00; continue;
                case 0xf1: field[n++] = 0xff; continue;
                default:   return ZERR_BADFIELD;
            }
        } else {
            field[n++] = *ptr++;
        }
    }
    if (*ptr)
        return (ZERR_BADFIELD);
    *len = n;
    return (ZERR_NONE);
}

Code_t
ZReadZcode32(const unsigned char *ptr,
             unsigned long *value_ptr)
{
    unsigned char buf[4];
    Code_t retval;
    int outlen;

    retval = ZReadZcode(ptr, buf, 4, &outlen);
    if (retval != ZERR_NONE)
        return retval;

    if (outlen != 4)
        return ZERR_BADFIELD;

    *value_ptr = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

    return ZERR_NONE;
}

Code_t
ZReadZcode16(const unsigned char *ptr,
             unsigned short *value_ptr)
{
    unsigned char buf[2];
    Code_t retval;
    int outlen;

    retval = ZReadZcode(ptr, buf, 2, &outlen);
    if (retval != ZERR_NONE)
        return retval;

    if (outlen != 2)
        return ZERR_BADFIELD;

    *value_ptr = (buf[0] << 8) | buf[1];
    return ZERR_NONE;
}

Code_t
ZReadZcodeAddr(const unsigned char *str,
               struct sockaddr_storage *addr)
{
    unsigned char addrbuf[sizeof(struct sockaddr_storage)];
    int len;
    int ret;

    ret = ZReadZcode(str, addrbuf,
                     sizeof(addrbuf), &len);
    if (ret != ZERR_NONE)
        return ret;

    if (len == sizeof(struct in6_addr)) {
        struct sockaddr_in6 *a6 = (struct sockaddr_in6 *)addr;
        a6->sin6_family = AF_INET6;
        memcpy(&a6->sin6_addr, addrbuf, len);
#ifdef HAvE_SOCKADDR_IN6_SIN6_LEN
        a6->sin6_len = sizeof(*a6);
#endif
    } else if (len == sizeof(struct in_addr)) {
        struct sockaddr_in *a4 = (struct sockaddr_in *)addr;
        a4->sin_family = AF_INET;
        memcpy(&a4->sin_addr, addrbuf, len);
#ifdef HAvE_SOCKADDR_IN6_SIN6_LEN
        a4->sin6_len = sizeof(*a4);
#endif
    } else
        return ZERR_BADFIELD;

    return ZERR_NONE;
}
