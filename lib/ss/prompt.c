/*
 * prompt.c: Routines for retrieving and setting a prompt.
 *
 * $Header: /srv/kcr/locker/zephyr/lib/ss/prompt.c,v 1.1 1993-10-12 02:50:32 probe Exp $
 * $Locker:  $
 *
 * Copyright 1987, 1988 by MIT Student Information Processing Board
 *
 * For copyright information, see copyright.h.
 */

#include "copyright.h"
#include <stdio.h>
#include "ss_internal.h"

static const char rcsid[] =
    "$Header: /srv/kcr/locker/zephyr/lib/ss/prompt.c,v 1.1 1993-10-12 02:50:32 probe Exp $";

ss_set_prompt(sci_idx, new_prompt)
     int sci_idx;
     char *new_prompt;
{
     ss_info(sci_idx)->prompt = new_prompt;
}

char *
ss_get_prompt(sci_idx)
     int sci_idx;
{
     return(ss_info(sci_idx)->prompt);
}