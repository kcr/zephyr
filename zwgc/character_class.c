/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/athena/zephyr/zwgc/character_class.c,v $
 *      $Author: marc $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#if (!defined(lint) && !defined(SABER))
static char rcsid_character_class_c[] = "$Header: /srv/kcr/athena/zephyr/zwgc/character_class.c,v 1.2 1989-11-02 01:55:18 marc Exp $";
#endif

#include <zephyr/mit-copyright.h>

#include "character_class.h"

static character_class cache;

character_class *string_to_character_class(str)
     string str;
{
    int i;

    bzero(cache, sizeof(cache));

    for (i=0; i<strlen(str); i++)
      cache[str[i]] = 1;

    return(&cache);
}
