/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/athena/zephyr/zwgc/character_class.c,v $
 *      $Author: probe $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#if (!defined(lint) && !defined(SABER))
static char rcsid_character_class_c[] = "$Id: character_class.c,v 1.5 1993-09-24 21:31:27 probe Exp $";
#endif

#include <zephyr/mit-copyright.h>
#include <zephyr/zephyr.h>
#include "character_class.h"

/* 
 * It may look like we are passing the cache by value, but since it's
 * really an array we are passing by reference.  C strikes again....
 */

static character_class cache;

/* character_class */
char * string_to_character_class(str)
     string str;
{
    int i;

    _BZERO(cache, sizeof(cache));

    for (i=0; i<strlen(str); i++)
      cache[str[i]] = 1;

    return(cache);
}
