/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/athena/zephyr/zwgc/xmark.h,v $
 *      $Author: marc $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#if (!defined(lint) && !defined(SABER))
static char rcsid_xmark_h[] = "$Header: /srv/kcr/athena/zephyr/zwgc/xmark.h,v 1.2 1989-11-02 01:58:06 marc Exp $";
#endif

#include <zephyr/mit-copyright.h>

#ifndef _XMARK_H_
#define _XMARK_H_

#define XMARK_START_BOUND 0
#define XMARK_END_BOUND 1
#define XMARK_TEMP_BOUND 2

#define XMARK_REDRAW_CURRENT 1
#define XMARK_REDRAW_OLD 2
#define XMARK_REDRAW_START 3
#define XMARK_REDRAW_END 4

#define xmarkStart(gram,x,y) xmarkSetBound(gram,x,y,XMARK_START_BOUND)
#define xmarkEnd(gram,x,y) xmarkSetBound(gram,x,y,XMARK_END_BOUND)

extern int markblock[];
extern int markchar[];
extern int markpixel[];
extern x_gram *markgram;

#define STARTBLOCK (markblock[XMARK_START_BOUND])
#define ENDBLOCK   (markblock[XMARK_END_BOUND])
#define STARTCHAR  (markchar[XMARK_START_BOUND])
#define ENDCHAR    (markchar[XMARK_END_BOUND])
#define STARTPIXEL (markpixel[XMARK_START_BOUND])
#define ENDPIXEL   (markpixel[XMARK_END_BOUND])

extern int xmarkSecond();
extern void xmarkRedraw();
extern void xmarkClear();
extern int xmarkExtendFromStart();
extern int xmarkExtendFromNearest();

#endif
