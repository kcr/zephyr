/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/athena/zephyr/zwgc/character_class.c,v $
 *      $Author: jtkohl $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#if (!defined(lint) && !defined(SABER))
static char rcsid_character_class_c[] = "$Id: character_class.c,v 1.4 1989-11-15 16:34:57 jtkohl Exp $";
#endif

#include <zephyr/mit-copyright.h>

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

    bzero(cache, sizeof(cache));

    for (i=0; i<strlen(str); i++)
      cache[str[i]] = 1;

    return(cache);
}
