/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/athena/zephyr/zwgc/xselect.h,v $
 *      $Author: ghudson $
 *	$Id: xselect.h,v 1.5 1997-09-14 22:15:02 ghudson Exp $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */


#include <zephyr/mit-copyright.h>

#ifndef _XSELECT_H_
#define _XSELECT_H_

extern void xicccmInitAtoms();
extern int xselGetOwnership();
extern int xselProcessSelection();
extern void xselOwnershipLost();
extern void xselGiveUpOwnership();

extern int xwmprotoDelete();

extern Atom XA_WM_PROTOCOLS,XA_WM_DELETE_WINDOW;

#endif
