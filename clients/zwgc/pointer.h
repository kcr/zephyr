/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/locker/zephyr/clients/zwgc/pointer.h,v $
 *      $Author: jtkohl $
 *	$Id: pointer.h,v 1.4 1989-11-15 10:43:47 jtkohl Exp $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */


#include <zephyr/mit-copyright.h>

#ifndef pointer_MODULE
#define pointer_MODULE

#ifdef __STDC__
typedef void *pointer;
#else
typedef char *pointer;
#endif

#endif
