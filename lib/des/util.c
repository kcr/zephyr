/*
 * $Source: /srv/kcr/locker/zephyr/lib/des/Attic/util.c,v $
 * $Author: ghudson $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 *
 * Miscellaneous debug printing utilities
 */

#include "mit-copyright.h"
#include "des.h"

#ifndef	lint
static const char rcsid_util_c[] =
    "$Id: util.c,v 1.3 1995-07-07 22:08:08 ghudson Exp $";
#endif

void des_cblock_print_file(x, fp)
    des_cblock x;
    FILE *fp;
{
    unsigned char *y = (unsigned char *) x;
    register int i = 0;
    fprintf(fp," 0x { ");

    while (i++ < 8) {
	fprintf(fp,"%x",*y++);
	if (i < 8)
	    fprintf(fp,", ");
    }
    fprintf(fp," }");
}
