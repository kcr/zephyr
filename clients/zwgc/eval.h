/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/locker/zephyr/clients/zwgc/eval.h,v $
 *      $Author: marc $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#if (!defined(lint) && !defined(SABER))
static char rcsid_eval_h[] = "$Header: /srv/kcr/locker/zephyr/clients/zwgc/eval.h,v 1.2 1989-11-02 01:55:52 marc Exp $";
#endif

#include <zephyr/mit-copyright.h>

#ifndef eval_MODULE
#define eval_MODULE

#include "new_string.h"

/*
 *    string eval_expr(Node *expr)
 *        Modifies: dict
 *        Requires: expr is a proper expression (NOT NULL).  (see node.c)
 *        Effects: Evaluates expr to its string value which is returned.
 *                 The returned string is on the heap and must be freed
 *                 eventually.
 */

extern string eval_expr();

/*
 *    int eval_bool_expr(Node *expr)
 *        Modifies: dict
 *        Requires: expr is a proper expression or NULL.  (see node.c)
 *        Effects: Evaluates expr to its boolean value which is returned.
 *                 NULL is defined to have the boolean value true.
 */

extern int eval_bool_expr();

#endif
