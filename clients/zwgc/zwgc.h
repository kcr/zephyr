/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/locker/zephyr/clients/zwgc/zwgc.h,v $
 *      $Author: marc $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#if (!defined(lint) && !defined(SABER))
static char rcsid_zwgc_h[] = "$Header: /srv/kcr/locker/zephyr/clients/zwgc/zwgc.h,v 1.2 1989-11-02 01:58:40 marc Exp $";
#endif

#include <zephyr/mit-copyright.h>

#ifdef DEBUG

extern int zwgc_debug;
#define dprintf(x)     if (zwgc_debug) printf(x)
#define dprintf1(x,y)     if (zwgc_debug) printf(x,y)

#else

#define dprintf(x)     
#define dprintf1(x,y)     

#endif
