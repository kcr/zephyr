/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZMakeAscii function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZMakeAscii.c,v $
 *	$Author: ghudson $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZMakeAscii.c,v 1.14 1995-07-08 02:47:17 ghudson Exp $ */

#include <internal.h>
#include <assert.h>

#ifndef lint
static const char rcsid_ZMakeAscii_c[] = "$Id: ZMakeAscii.c,v 1.14 1995-07-08 02:47:17 ghudson Exp $";
#endif

Code_t ZMakeAscii(ptr, len, field, num, proto_num)
    register char *ptr;
    int len;
    unsigned char *field;
    int num;
    int proto_num;
{
    int i;
    register char *itox_chars = "0123456789ABCDEF";

    assert(num >= proto_num);

    /* Skip higher-order bytes if field length is greater than proto length. */
    if (num > proto_num) {
	field += (num - proto_num);
	num = proto_num;
    }
    for (i=0;i<num;i++) {
	/* we need to add "0x" if we are between 4 byte pieces */
	if ((i & 3) == 0) {
	    if (len < (i?4:3))
		return ZERR_FIELDLEN;
	    /* except at the beginning, put a space in before the "0x" */
	    if (i) {
		*ptr++ = ' ';
		len--;
	    }
	    *ptr++ = '0';
	    *ptr++ = 'x';
	    len -= 2;
	} 
	if (len < 3)
	    return ZERR_FIELDLEN;
	*ptr++ = itox_chars[(int) (field[i] >> 4)];
	*ptr++ = itox_chars[(int) (field[i] & 0xf)];
	len -= 2;
    }

    *ptr = '\0';
    return ZERR_NONE;
}
