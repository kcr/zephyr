/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/locker/zephyr/clients/zwgc/buffer.c,v $
 *      $Author: marc $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#if (!defined(lint) && !defined(SABER))
static char rcsid_buffer_c[] = "$Header: /srv/kcr/locker/zephyr/clients/zwgc/buffer.c,v 1.2 1989-11-02 01:55:12 marc Exp $";
#endif

#include <zephyr/mit-copyright.h>

#include "new_memory.h"
#include "buffer.h"

static char *buffer = 0;

string buffer_to_string()
{
    return(buffer);
}

void clear_buffer()
{
    if (buffer)
      free(buffer);

    buffer = string_Copy("");
}

void append_buffer(str)
     char *str;
{
    buffer = string_Concat2(buffer, str);
}
