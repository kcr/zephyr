/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZGetLocations function.
 *
 *	Created by:	Robert French
 *
 *	$Id: ZGetLocs.c,v 1.7 1999-01-22 23:19:11 ghudson Exp $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#ifndef lint
static char rcsid_ZGetLocations_c[] = "$Id: ZGetLocs.c,v 1.7 1999-01-22 23:19:11 ghudson Exp $";
#endif

#include <internal.h>

#define min(a,b) ((a)<(b)?(a):(b))
	
Code_t ZGetLocations(location, numlocs)
    ZLocations_t *location;
    int *numlocs;
{
    int i;
	
    if (!__locate_list)
	return (ZERR_NOLOCATIONS);

    if (__locate_next == __locate_num)
	return (ZERR_NOMORELOCS);
	
    for (i=0;i<min(*numlocs, __locate_num-__locate_next);i++)
	location[i] = __locate_list[i+__locate_next];

    if (__locate_num-__locate_next < *numlocs)
	*numlocs = __locate_num-__locate_next;

    __locate_next += *numlocs;
	
    return (ZERR_NONE);
}
