/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/athena/zephyr/zwgc/text_operations.h,v $
 *      $Author: marc $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#if (!defined(lint) && !defined(SABER))
static char rcsid_text_operations_h[] = "$Header: /srv/kcr/athena/zephyr/zwgc/text_operations.h,v 1.2 1989-11-02 01:57:39 marc Exp $";
#endif

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
