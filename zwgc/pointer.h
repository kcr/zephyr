/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/athena/zephyr/zwgc/pointer.h,v $
 *      $Author: marc $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#if (!defined(lint) && !defined(SABER))
static char rcsid_pointer_h[] = "$Header: /srv/kcr/athena/zephyr/zwgc/pointer.h,v 1.2 1989-11-02 01:57:00 marc Exp $";
#endif

#include <zephyr/mit-copyright.h>

#ifndef pointer_MODULE
#define pointer_MODULE

#if defined(mips) && defined(ultrix)
typedef char *pointer;
#else
typedef void *pointer;
#endif

#endif
