/* This file is part of the Project Athena Zephyr Notification System.
 * It is one of the source files comprising zwgc, the Zephyr WindowGram
 * client.
 *
 *      Created by:     Marc Horowitz <marc@athena.mit.edu>
 *
 *      $Source: /srv/kcr/athena/zephyr/zwgc/error.h,v $
 *      $Author: marc $
 *
 *      Copyright (c) 1989 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h".
 */

#if (!defined(lint) && !defined(SABER))
static char rcsid_error_h[] = "$Header: /srv/kcr/athena/zephyr/zwgc/error.h,v 1.2 1989-11-02 01:55:43 marc Exp $";
#endif

#include <zephyr/mit-copyright.h>

#ifndef error_MODULE
#define error_MODULE

#include <stdio.h>

extern int errno;
extern int error_code;
extern void com_err();

#define  FATAL_TRAP(function_call, message) \
                                           { error_code = (function_call);\
					    if (error_code) {\
						com_err("zwgc", error_code,\
							message);\
						exit(3);\
					    }\
					   }

#define  TRAP(function_call, message) \
                                          { error_code = (function_call);\
					    if (error_code) {\
						com_err("zwgc", error_code,\
							message);\
					    }\
					   }

#define  ERROR(a)                { fprintf(stderr, "zwgc: "); \
				   fprintf(stderr, a);\
				   fflush(stderr); }

#define  ERROR2(a,b)             { fprintf(stderr, "zwgc: "); \
				   fprintf(stderr, a, b);\
				   fflush(stderr); }

#define  ERROR3(a,b,c)           { fprintf(stderr, "zwgc: "); \
				   fprintf(stderr, a, b, c);\
				   fflush(stderr); }

#define  ERROR4(a,b,c,d)         { fprintf(stderr, "zwgc: "); \
				   fprintf(stderr, a, b, c, d);\
				   fflush(stderr); }

#endif
