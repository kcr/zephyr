/* This file is part of the Project Athena Zephyr Notification System.
 * It contains functions for managing multiple timeouts.
 *
 *	Created by:	John T. Kohl
 *	Derived from timer_manager_ by Ken Raeburn
 *
 *	$Source: /srv/kcr/locker/zephyr/server/timer.c,v $
 *	$Author: jtkohl $
 *
 */

#ifndef lint
static char rcsid_timer_c[] = "$Header: /srv/kcr/locker/zephyr/server/timer.c,v 1.2 1987-06-12 10:14:03 jtkohl Exp $";
#endif lint

/*
 * timer_manager_ -- routines for handling timers in login_shell
 * (and elsewhere)
 *
 * Copyright 1986 Student Information Processing Board,
 * Massachusetts Institute of Technology
 *
 * written by Ken Raeburn

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. and the Student
Information Processing Board not be used in
advertising or publicity pertaining to distribution of the
software without specific, written prior permission.
M.I.T. and the Student Information Processing Board
make no representations about the suitability of
this software for any purpose.  It is provided "as is"
without express or implied warranty.

 */

#include <stdio.h>
#include "zserver.h"

/* Maximum simultaneous triggers.. */
/* This is how many alarms we will process between each select() poll */
#define MAXSIMUL (16)

long nexttimo = 0L;			/* the Unix time of the next
					   alarm */
static timer timers = NULL;
static long right_now;

char *calloc(), *malloc(), *realloc();
static void timer_botch();
static void insert_timer();

/*
 * timer_set_rel(time_rel, proc)
 *   time_rel: alarm time relative to now, in seconds
 *   proc: subroutine to be called (no args, returns void)
 *
 * creates a "timer" and adds it to the current list, returns "timer"
 */

timer timer_set_rel (time_rel, proc, arg)
     long time_rel;
     void (*proc)();
     caddr_t arg;
{
	timer new_t;
	right_now = NOW;
	new_t = (timer) malloc(TIMER_SIZE);
	if (new_t == NULL) return(NULL);
	ALARM_TIME(new_t) = time_rel + right_now;
	ALARM_FUNC(new_t) = proc;
	ALARM_NEXT(new_t) = NULL;
	ALARM_PREV(new_t) = NULL;
	ALARM_ARG(new_t)  = arg;
	add_timer(new_t);
	return(new_t);
}

/*
 * timer_set_abs (time_abs, proc, arg)
 *   time_abs: alarm time, absolute
 *   proc: routine to call when timer expires
 *   arg:  argument to routine
 *
 * functions like timer_set_rel
 */

timer timer_set_abs (time_abs, proc, arg)
     long time_abs;
     void (*proc)();
     caddr_t arg;
{
	timer new_t;

	new_t = (timer)malloc(TIMER_SIZE);
	if (new_t == NULL) return(NULL);
	ALARM_TIME(new_t) = time_abs;
	ALARM_FUNC(new_t) = proc;
	ALARM_NEXT(new_t) = NULL;
	ALARM_PREV(new_t) = NULL;
	ALARM_ARG(new_t)  = arg;
	add_timer(new_t);
	return(new_t);
}

/*
 * timer_reset
 *
 * args:
 *   tmr: timer to be removed from the list
 *
 * removes any timers matching tmr and reallocates list
 *
 */

void
timer_reset(tmr)
     timer tmr;
{
	if (!ALARM_PREV(tmr) || !ALARM_NEXT(tmr)) {
		if (zdebug)
			syslog(LOG_DEBUG, "reset_timer() of unscheduled timer\n");
		return(-1);
	}
	if (tmr == timers) {
		if (zdebug)
			syslog(LOG_DEBUG, "reset_timer of timer head\n");
		return(-1);
	}
	right_now = NOW;
	remque(tmr);
	ALARM_PREV(tmr) = NULL;
	ALARM_NEXT(tmr) = NULL;
	free(tmr);
	return;
}


#define set_timeval(t,s) ((t).tv_sec=(s),(t).tv_usec=0,(t))

/* add_timer(t:timer)
 *
 * args:
 *   t: new "timer" to be added
 *
 * returns:
 *   0 if successful
 *   -1 if error (errno set) -- old time table may have been destroyed
 *
 */
static void
add_timer(new_t)
     timer new_t;
{
	if (ALARM_PREV(new_t) || ALARM_NEXT(new_t)) {
		if (zdebug)
			syslog(LOG_DEBUG, "add_timer of enqueued timer\n");
		return(-1);
	}
	right_now = NOW;
	insert_timer(new_t);
	return;
}

/*
 * insert_timer(t:timer)
 *
 * inserts a timer into the current timer table.
 *
 */

static void
insert_timer(new_t)
     timer new_t;
{
	register timer t;

	if (timers == NULL) {
		timers = (timer) malloc(TIMER_SIZE);
		ALARM_NEXT(timers) = timers;
		ALARM_PREV(timers) = timers;
		ALARM_TIME(timers) = 0L;
		ALARM_FUNC(timers) = timer_botch;
	}
	for (t = ALARM_NEXT(timers); t != timers; t = ALARM_NEXT(t)) {
		if (ALARM_TIME(t) > ALARM_TIME(new_t)) {
			insque(new_t, ALARM_PREV(t));
			return;
		}
	}
	insque(new_t, ALARM_PREV(timers));
}

/*
 * timer_process -- checks for next timer execution time
 * and execute 
 *
 */

void
timer_process()
{
	register int i;
	register struct _timer *t;
	void (*queue[MAXSIMUL])();
	caddr_t queue_arg[MAXSIMUL];
	int nqueue=0;

	for (t=ALARM_NEXT(timers); t != timers && right_now >= ALARM_TIME(t); 
	     t=ALARM_NEXT(t)) {
		/*
		 * This one goes off NOW..
		 * Enqueue the function, and delete the timer.
		 */
		register timer s;
     
		if (nqueue>MAXSIMUL) break;
		queue_arg[nqueue] = ALARM_ARG(t);
		queue[nqueue++]=ALARM_FUNC(t);
		remque(t); 
		s = t;
		t = ALARM_PREV(t);
	        ALARM_PREV(s) = NULL;
		ALARM_NEXT(s) = NULL;
	}
	/* note that in the case that there are no timers, the ALARM_TIME
	   is set to 0L, which is what the main loop expects as the
	   nexttimo when we have no timout work to do */
	nexttimout = ALARM_TIME(t);
	
	for (i=0; i < nqueue; i++)
		(queue[i])(queue_arg[i]);
	return;
}

static void
timer_botch()
{
	syslog(LOG_DEBUG, "Timer botch\n");
	abort();
}

void
print_timers()
{
	register timer t;

	printf("\nIt's currently %d\n", NOW);
	for (t=ALARM_NEXT(timers); t != timers; t = ALARM_NEXT(t)) {
		printf("Timer %x: time %d\n", t, ALARM_TIME(t));
	}
}
