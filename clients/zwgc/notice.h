/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/locker/zephyr/clients/zwgc/notice.h,v $
 *      $Author: marc $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#if (!defined(lint) && !defined(SABER))
static char rcsid_notice_h[] = "$Header: /srv/kcr/locker/zephyr/clients/zwgc/notice.h,v 1.2 1989-11-02 01:56:48 marc Exp $";
#endif

#include <zephyr/mit-copyright.h>

#ifndef notice_MODULE
#define notice_MODULE

#include "new_string.h"

/*
 *    int count_nulls(char *data, int length)
 *        Requires: length>=0
 *        Effects: Returns the # of nulls in data[0]..data[length-1]
 */

extern int count_nulls();

/*
 *    string get_next_field(char **data_p, int *length_p)
 *        Requires: *length_p >= 0
 *        Modifies: *data_p, *length_p
 *        Effects: Treats (*data_p)[0], (*data_p)[1], ... (*data_p)[length-1]
 *                 as a series of null-seperated fields.  This function
 *                 returns a copy of the first field on the heap.  This
 *                 string must eventually be freed.  Also, *data_p is
 *                 advanced and *length_p decreased so that another
 *                 call to this procedure with the same arguments will
 *                 return the second field.  The next call will return
 *                 the third field, etc.  "" is returned if 0 fields
 *                 remain.  (this is the case when *length_p == 0)
 */

extern string get_next_field();

/*
 *    string get_field(char *data, int length, int num)
 *        Requires: length>=0, num>0
 *        Effects: Treats data[0]..data[length-1] as a series of
 *                 null-seperated fields.  This function returns a copy of
 *                 the num'th field (numbered from 1 in this case) on the
 *                 heap.  This string must eventually be freed.  If there
 *                 is no num'th field (because num<1 or num># of fields),
 *                 "" is returned.
 */

extern string get_field();

/*
 *    string convert_nulls_to_newlines(data, length)
 *       Requires: length>=0, malloc never returns NULL
 *       Effects: Takes data[0]..data[length-1], converts all nulls to
 *                newlines ('\n') and returns the result as a null-terminated
 *                string on the heap.  The returned string must eventually
 *                be freed.
 */

extern string convert_nulls_to_newlines();


extern char *decode_notice();

#endif
