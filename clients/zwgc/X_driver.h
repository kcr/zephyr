#ifndef x_driver_MODULE
#define x_driver_MODULE

/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/locker/zephyr/clients/zwgc/X_driver.h,v $
 *      $Author: marc $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#if (!defined(lint) && !defined(SABER))
static char rcsid_X_driver_h[] = "$Header: /srv/kcr/locker/zephyr/clients/zwgc/X_driver.h,v 1.3 1989-11-02 02:19:36 marc Exp $";
#endif

#include <zephyr/mit-copyright.h>

#include <X11/Xlib.h>

extern Display *dpy;

extern char *get_string_resource();
extern int get_bool_resource();

#endif
