/* This file is part of the Project Athena Zephyr Notification System.
 * It contains code for the "zleave" command.
 *
 *      Created by:     David Jedlinsky
 *
 *      $Id$
 *
 *      Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h". 
 */

#include <sysdep.h>
#include <zephyr/mit-copyright.h>
#include <zephyr/zephyr.h>

#include <com_err.h>

#ifndef lint
static const char rcsid_zlocate_c[] = "$Id$";
#endif /* lint */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific written prior permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#define MESSAGE_CLASS "MESSAGE"
#define INSTANCE "LEAVE"
/*
 * zleave [[+]hhmm [ -m "Reminder Message" ]]
 *  or
 * zleave can[cel]
 *
 * Reminds you when you have to leave.
 * Leave prompts for input and goes away if you hit return.
 * Messages are sent through Zephyr.  Subscriptions are handled automagically.
 * It nags you like a mother hen.
 */
char origlogin[20];
char tempfile[40];
char *whenleave;
char *reminder_message = NULL;
char buff[100];
int use_zephyr=1, oldpid;

void usage(void);
void doalarm(long);
void bother(long, char *);
void delay(long);
int gethm(char *, int *, int*);

int
main(int argc,
     char **argv)
{
	time_t now;
	long when, diff;
	int hours, minutes;
	char *cp;
	FILE *fp;
	struct tm *nv;
	int port, c;
	ZSubscription_t sub;
	
	if (ZInitialize() != ZERR_NONE) {
	      fprintf(stderr,"No Zephyr! Will write directly to terminal.\n");
	      use_zephyr = 0;
	}
	(void) sprintf(tempfile, "/tmp/zleave.%d", (int) getuid());

	if (use_zephyr) {
		if ((port = ZGetWGPort()) == -1) {
			fprintf(stderr,
				"Can't find WindowGram subscription port.\n");
			fprintf(stderr,"Will write directly to terminal.\n");
			use_zephyr = 0;
		} else {
			sub.zsub_class = MESSAGE_CLASS;
			sub.zsub_classinst = INSTANCE;
			sub.zsub_recipient = ZGetSender();
			if (ZSubscribeToSansDefaults(&sub,1,(u_short)port)
			    != ZERR_NONE) {
				fprintf(stderr,
					"Subscription error!  Writing to your terminal...\n");
				use_zephyr = 0;
			} 
		}
	}	
	if (!use_zephyr) {
	    if ((cp = getlogin()) == NULL) {
		fputs("leave: You are not logged in.\n", stderr);
		exit(1);
	    }
	    (void) strcpy(origlogin, cp);
	}

	c = 1;
	while ((c<argc) && (! reminder_message))
	    if (!strcmp(argv[c++],"-m")) {
		if (argv[c])
		    reminder_message = argv[c];
		else
		    usage();
	    }

	if (!reminder_message)
	    reminder_message = "";

	if (argc < 2) {
		printf("When do you have to leave? ");
		(void) fflush(stdout);
		buff[read(0, buff, sizeof buff)] = 0;
		cp = buff;
	} else
		cp = argv[1];
	if (*cp == '\n')
		exit(0);
	if (*cp == '+') {
		cp++;
		if (!gethm(cp, &hours, &minutes))
			usage();
		if (minutes < 0 || minutes > 59)
			usage();
		diff = 60*hours+minutes;
		doalarm(diff);
		exit(0);
	}
	if (!strcmp(cp, "cancel") || !strcmp(cp, "can")) {
	      if (!(fp = fopen(tempfile,"r"))) {
		    printf("No zleave is currently running.\n");
		    exit(0);
	      }
	      if (fscanf(fp, "%d", &oldpid) != 1) {
		      printf("The zleave pid file is corrupted.\n");
		      (void) fclose(fp);
		      exit(0);
	      }
	      (void) fclose(fp);
	      if (kill(oldpid,9))
		    printf("No zleave is currently running.\n");
	      (void) unlink(tempfile);
	      exit(0);
	}
	if (!gethm(cp, &hours, &minutes))
		usage();
	if (hours > 12)
		hours -= 12;
	if (hours == 12)
		hours = 0;

	if (hours < 0 || hours > 12 || minutes < 0 || minutes > 59)
		usage();

	(void) time(&now);
	nv = localtime(&now);
	when = 60*hours+minutes;
	if (nv->tm_hour > 12)
		nv->tm_hour -= 12;	/* do am/pm bit */
	now = 60 * nv->tm_hour + nv->tm_min;
	diff = when - now;
	while (diff < 0)
		diff += 12*60;
	if (diff > 11*60) {
		fprintf(stderr, "That time has already passed!\n");
		exit(1);
	}

	doalarm(diff);
	exit(0);
}

void
usage(void)
{
	fprintf(stderr, "usage: zleave [[+]hhmm [-m \"Reminder Message\"]]\n\
\tor: zleave can[cel]\n");
	exit(1);
}

int
gethm(char *cp,
      int *hp,
      int *mp)
{
	char c;
	int tod;

	tod = 0;
	while ((c = *cp++) != '\0') {
		if (!isdigit(c))
			return(0);
		tod = tod * 10 + (c - '0');
	}
	*hp = tod / 100;
	*mp = tod % 100;
	return(1);
}

void
doalarm(long nmins)
{
	time_t daytime;
	char *msg1, *msg2, *msg3, *msg4;
	register int i;
	long slp1, slp2, slp3, slp4;
	long seconds, gseconds;
	FILE *fp;
#ifdef _POSIX_VERSION
	struct sigaction sa;
#endif

	seconds = 60 * nmins;
	if (seconds <= 0)
		seconds = 1;
	gseconds = seconds;

	msg1 = "You have to leave in 5 minutes";
	if (seconds <= 60*5) {
		slp1 = 0;
	} else {
		slp1 = seconds - 60*5;
		seconds = 60*5;
	}

	msg2 = "Just one more minute!";
	if (seconds <= 60) {
		slp2 = 0;
	} else {
		slp2 = seconds - 60;
		seconds = 60;
	}

	msg3 = "Time to leave!";
	slp3 = seconds;

	msg4 = "You're going to be late!";
	slp4 = 60L;

	(void) time(&daytime);
	daytime += gseconds;
	whenleave = ctime(&daytime);

	fp = fopen(tempfile,"r");
	if (fp) {
	      if (fscanf(fp, "%d", &oldpid) == 1)
		      if (!kill(oldpid,9))
			      printf("Old zleave process killed.\n");
	      (void) fclose(fp);
	}
	printf("Alarm set for %s", whenleave);

/* Subscribe to MESSAGE.LEAVE here */

	switch(fork()) {
	    case -1:
	      perror("fork");
	      exit(-1);
	      break;
	    case 0:
	      break;
	    default:
	      exit(0);
	      break;
	}
	if (!(fp = fopen(tempfile, "w")))
	  fprintf(stderr, "Cannot open pid file.\n");
	else {
	      fprintf(fp, "%d\n", getpid());
	      if (fclose(fp) == EOF)
		      (void) perror("fclose on pid file");
	}

#ifdef _POSIX_VERSION
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sa, (struct sigaction *)0);
	sigaction(SIGQUIT, &sa, (struct sigaction *)0);
	sigaction(SIGTERM, &sa, (struct sigaction *)0);
	sigaction(SIGTTOU, &sa, (struct sigaction *)0);
#else
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);
	(void) signal(SIGTTOU, SIG_IGN);
#endif

	if (slp1)
		bother(slp1, msg1);
	if (slp2)
		bother(slp2, msg2);
	bother(slp3, msg3);
	for (i = 0; i < 10; i++)
		bother(slp4, msg4);

	bother(0L, "That was the last time I'll tell you. Bye.");
	(void) unlink(tempfile);
	exit(0);
}

void
bother(long slp,
       char *msg)
{
      ZNotice_t notice;
      ZNotice_t retnotice;
      int retval;
      char *real_message;

      delay(slp);

      if (use_zephyr) {
	    real_message = (char *) malloc(strlen(msg) +
					   strlen(reminder_message) + 3);
	    if (real_message == NULL) {
		fprintf (stderr, "zleave: out of memory\n");
		exit (1);
	    }
	    sprintf(real_message,"%c%s\n%s",'\0',msg,reminder_message);

	    (void) memset((char *)&notice, 0, sizeof(notice));
	    notice.z_kind = ACKED;
	    notice.z_port = 0;
	    notice.z_class = MESSAGE_CLASS;
	    notice.z_class_inst = INSTANCE;
	    notice.z_recipient = ZGetSender();
	    notice.z_opcode = "";
	    notice.z_sender = (char *) 0;
	    notice.z_default_format = "\n$2";
	    notice.z_message = real_message;
	    /* +3: initial null, newline, final null */
	    notice.z_message_len = strlen(msg)+strlen(reminder_message)+3;
	    
	    if (ZSendNotice(&notice, ZAUTH) != ZERR_NONE) {
		  printf("\7\7\7%s\n%s", msg, reminder_message);
		  use_zephyr = 0;
	    } else
	    if ((retval = ZIfNotice(&retnotice, (struct sockaddr_in *) 0,
				    ZCompareUIDPred, 
				    (char *)&notice.z_uid)) != ZERR_NONE) {
		fprintf(stderr,
			"zleave: %s while waiting for acknowledgement\n", 
			error_message(retval));
		use_zephyr = 0;
	    } else
	    if (retnotice.z_kind == SERVNAK) {
		fprintf(stderr,
			"zleave: authorization failure while sending\n");
		use_zephyr = 0;
	    } else
	    if (retnotice.z_kind != SERVACK || !retnotice.z_message_len) {
		fprintf(stderr, "zleave: Detected server failure while receiving acknowledgement\n");
		use_zephyr = 0;
	    } else
	    if (strcmp(retnotice.z_message, ZSRVACK_SENT)) {
		/* it wasn't sent */
		exit(0);
	    }
	    if (!use_zephyr)
		exit(1);
	    ZFreeNotice(&retnotice);
	    free(real_message);
      } else
#ifdef __STDC__
	printf("\a\a\a%s\n%s", msg, reminder_message);
#else
	printf("\7\7\7%s\n%s", msg, reminder_message);
#endif
}

/*
 * delay is like sleep but does it in 100 sec pieces and
 * knows what zero means.
 */
void
delay(long secs)
{
	long n;
	register char *l;

	while (secs > 0) {
		n = 100;
		if (secs < n)
			n = secs;
		secs -= n;
		if (n > 0)
			sleep((unsigned) n);
		if (!use_zephyr) {
		    l = getlogin();
		    if (l == NULL)
			exit(0);
		    if (strcmp(origlogin, l) != 0)
			exit(0);
		}
	}
}

#ifndef HAVE_GETLOGIN
char *getlogin() {
#include <utmp.h>

	static struct utmp ubuf;
	int ufd;

	ufd = open("/etc/utmp",0);
	seek(ufd, ttyn(0)*sizeof(ubuf), 0);
	read(ufd, &ubuf, sizeof(ubuf));
	ubuf.ut_name[sizeof(ubuf.ut_name)] = 0;
	return(&ubuf.ut_name);
}
#endif
