/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/locker/zephyr/clients/zwgc/Attic/ulong.h,v $
 *      $Author: jfc $
 *	$Id: ulong.h,v 1.3 1991-06-20 09:18:32 jfc Exp $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */
#ifndef ulong_MODULE
#define ulong_MODULE

#if defined(_AIX) || defined(_AUX_SOURCE) || defined(SYSV)
/* Sys5 derived systems define ulong in <sys/types.h>.  */
#include <sys/types.h>
#else
typedef unsigned long ulong;
#endif

#endif
