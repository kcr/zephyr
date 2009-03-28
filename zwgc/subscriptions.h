/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Id$
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */


#include <zephyr/mit-copyright.h>

#ifndef subscriptions_MODULE
#define subscriptions_MODULE

extern int zwgc_active;

extern int puntable_address_p(string, string, string);
extern void punt(string, string, string);
extern void unpunt(string, string, string);
extern void zwgc_shutdown(void);
extern void zwgc_startup(void);

#define USRSUBS ".zephyr.subs"

#endif
