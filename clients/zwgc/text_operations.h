/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/locker/zephyr/clients/zwgc/text_operations.h,v $
 *      $Author: jtkohl $
 *	$Id: text_operations.h,v 1.3 1989-11-08 14:36:33 jtkohl Exp $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */


#include <zephyr/mit-copyright.h>

#ifndef text_operations_MODULE
#define text_operations_MODULE

#include "character_class.h"

extern string protect();
extern string verbatim();
extern string lany();
extern string lbreak();
extern string lspan();
extern string rany();
extern string rbreak();
extern string rspan();

#endif
