/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZMakeAscii function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZMakeAscii.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZMakeAscii.c,v 1.8 1988-06-23 10:31:46 jtkohl Exp $ */

#ifndef lint
static char rcsid_ZMakeAscii_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZMakeAscii.c,v 1.8 1988-06-23 10:31:46 jtkohl Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

Code_t ZMakeAscii(ptr, len, field, num)
    char *ptr;
    int len;
    unsigned char *field;
    int num;
{
    int i;

    for (i=0;i<num;i++) {
	if (!(i%4)) {
	    if (len < 3+(i!=0))
		return (ZERR_FIELDLEN);
	    if (i) {
		*ptr++ = ' ';
		len--;
	    }
	    *ptr++ = '0';
	    *ptr++ = 'x';
	    len -= 2;
	} 
	if (len < 3)
	    return (ZERR_FIELDLEN);
	*ptr++ = cnvt_itox((int) (field[i] >> 4));
	*ptr++ = cnvt_itox((int) (field[i] & 0xf));
	len -= 2;
    }

    *ptr = '\0';
    return (ZERR_NONE);
}

cnvt_itox(i)
    int i;
{
    i += '0';
    if (i <= '9')
	return (i);
    i += 'A'-'9'-1;
    return (i);
}
