/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZReadAscii function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZReadAscii.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZReadAscii.c,v 1.12 1988-06-23 10:32:47 jtkohl Exp $ */

#ifndef lint
static char rcsid_ZReadAscii_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZReadAscii.c,v 1.12 1988-06-23 10:32:47 jtkohl Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

int ZReadAscii(ptr, len, field, num)
    char *ptr;
    int len;
    unsigned char *field;
    int num;
{
    int i;
    unsigned int hexbyte;
    register char c1, c2;

    for (i=0;i<num;i++) {
	if (*ptr == ' ') {
	    ptr++;
	    if (--len < 0)
		return (ZERR_BADFIELD);
	} 
	if (ptr[0] == '0' && ptr[1] == 'x') {
	    ptr += 2;
	    len -= 2;
	    if (len < 0)
		return (ZERR_BADFIELD);
	} 
	c1 = cnvt_xtoi(ptr[0]);
	c2 = cnvt_xtoi(ptr[1]);
	if (c1 < 0 || c2 < 0)
		return(ZERR_BADFIELD);
	hexbyte = (c1 << 4) | c2;
	field[i] = hexbyte;
	ptr += 2;
	len -= 2;
	if (len < 0)
	    return (ZERR_BADFIELD);
    }

    if (*ptr)
	return (ZERR_BADFIELD);

    return (ZERR_NONE);
}

cnvt_xtoi(c)
    char c;
{
    c -= '0';
    if (c < 10)
	return (c);
    c -= 'A'-'9'-1;
    if (c < 16)
	return (c);
    c -= 'a'-'A';
    if (c > 15)
	return (-1);
    return (c);
}
