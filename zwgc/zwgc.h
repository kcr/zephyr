/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Id$
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */


#include <zephyr/mit-copyright.h>

#ifdef DEBUG

extern int zwgc_debug;
#define dprintf(x)     if (zwgc_debug) printf(x)
#define dprintf1(x,y)     if (zwgc_debug) printf(x,y)

#else

#define dprintf(x)     
#define dprintf1(x,y)     

#endif
