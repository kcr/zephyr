#ifndef __HM_H__
#define __HM_H__
/* This file is part of the Project Athena Zephyr Notification System.
 * It contains the hostmanager header file.
 *
 *      Created by:     David C. Jedlinsky
 *
 *      $Source: /srv/kcr/locker/zephyr/zhm/zhm.h,v $
 *      $Author: opus $
 *      $Header: /srv/kcr/locker/zephyr/zhm/zhm.h,v 1.2 1987-07-01 03:24:42 opus Exp $
 *
 *      Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>
#include <zephyr/zephyr.h>
#include <syslog.h>

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
#define NOTICE_TIMEOUT 10
#define BOOTING 1
#define NOTICES 2

#define MAXRETRIES 5

extern char *malloc();

#endif !__HM_H__
