#ifndef __HM_H__
#define __HM_H__
/* This file is part of the Project Athena Zephyr Notification System.
 * It contains the hostmanager header file.
 *
 *      Created by:     David C. Jedlinsky
 *
 *      $Source: /srv/kcr/athena/zephyr/zhm/zhm.h,v $
 *      $Author: jtkohl $
 *      $Header: /srv/kcr/athena/zephyr/zhm/zhm.h,v 1.7 1988-06-23 14:39:32 jtkohl Exp $
 *
 *      Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>
#include <zephyr/zephyr.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netdb.h>
#ifdef lint
#include <sys/uio.h>			/* make lint shut up */
#endif /* lint */

#ifdef DEBUG
#define DPR(a) fprintf(stderr, a); fflush(stderr)
#define DPR2(a,b) fprintf(stderr, a, b); fflush(stderr)
#define Zperr(e) fprintf(stderr, "Error = %d\n", e)
#else
#define DPR(a)
#define DPR2(a,b)
#define Zperr(e)
#endif

#define ever (;;)

#define SERV_TIMEOUT 20
#define NOTICE_TIMEOUT 25
#define BOOTING 1
#define NOTICES 2

#define MAXRETRIES 2

extern char *malloc();
extern Code_t send_outgoing();

#ifdef vax
#define MACHINE "vax"
#define ok
#endif vax
#ifdef ibm032
#define MACHINE "rt"
#define ok
#endif ibm032
#ifndef ok
#define MACHINE "unknown"
#endif ok

#endif !__HM_H__
