/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/athena/zephyr/zwgc/Attic/ulong.h,v $
 *      $Author: ghudson $
 *	$Id: ulong.h,v 1.5 1994-11-01 16:08:49 ghudson Exp $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */
#ifndef ulong_MODULE
#define ulong_MODULE

#include <sys/types.h>

#if defined(ultrix) || defined(vax) || defined(SUNOS)
typedef unsigned long ulong;
#endif

#endif
