/* This file is part of the Project Athena Zephyr Notification System.
 * It contains the hostmanager client program.
 *
 *      Created by:     David C. Jedlinsky
 *
 *      $Source: /srv/kcr/locker/zephyr/zhm/zhm.c,v $
 *      $Author: ghudson $
 *
 *      Copyright (c) 1987,1991 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h". 
 */

#include "zhm.h"

static const char rcsid_hm_c[] = "$Id: zhm.c,v 1.58 1995-07-07 22:17:25 ghudson Exp $";

#ifdef ZEPHYR_USES_HESIOD
int ZEPHYR_USES_hesiod = 0;
#endif

#ifdef macII
#define srandom srand48
#endif

#ifdef _PATH_VARRUN
#define PIDDIR _PATH_VARRUN
#else
#define PIDDIR "/etc/"
#endif

int hmdebug, rebootflag, errflg, dieflag, inetd, oldpid, nofork;
int no_server = 1, nservchang, nserv, nclt;
int booting = 1, timeout_type, deactivated = 1;
long starttime;
u_short cli_port;
struct sockaddr_in cli_sin, serv_sin, from;
int numserv;
char **serv_list = NULL;
char prim_serv[MAXHOSTNAMELEN], cur_serv[MAXHOSTNAMELEN];
char *zcluster;
int sig_type;
struct hostent *hp;
char **clust_info;
char hostname[MAXHOSTNAMELEN], loopback[4];
char PidFile[128];

static void choose_server __P((void));
static void init_hm __P((void));
static void detach __P((void));
static void handle_timeout __P((void));
static void send_stats __P((ZNotice_t *, struct sockaddr_in *));
static char *strsave __P((const char *));

RETSIGTYPE set_sig_type(sig)
     int sig;
{
     sig_type = sig;
}

main(argc, argv)
char *argv[];
{
     ZNotice_t notice;
     ZPacket_t packet;
     Code_t ret;
     int opt, pak_len;
     extern int optind;
     register int i, j = 0;

     sprintf(PidFile, "%szhm.pid", PIDDIR);

     if (gethostname(hostname, MAXHOSTNAMELEN) < 0) {
	  printf("Can't find my hostname?!\n");
	  exit(-1);
     }
     prim_serv[0] = '\0';
     while ((opt = getopt(argc, argv, "drhin")) != EOF)
	  switch(opt) {
	  case 'd':
	       hmdebug++;
	       break;
	  case 'h':
	       /* Die on SIGHUP */
	       dieflag++;
	       break;
	  case 'r':
	       /* Reboot host -- send boot notice -- and exit */
	       rebootflag++;
	       break;
	  case 'i':
	       /* inetd operation: don't do bind ourselves, fd 0 is
		  already connected to a socket. Implies -h */
	       inetd++;
	       dieflag++;
	       break;
	  case 'n':
	       nofork = 1;
	       break;
	  case '?':
	  default:
	       errflg++;
	       break;
	  }
     if (errflg) {
	  fprintf(stderr, "Usage: %s [-d] [-h] [-r] [-n] [server]\n", argv[0]);
	  exit(2);
     }

     numserv = 0;

     /* Override server argument? */
     if (optind < argc) {
	 if ((hp = gethostbyname(argv[optind++])) == NULL) {
	     printf("Unknown server name: %s\n", argv[optind-1]);
	 } else
	     (void) strcpy(prim_serv, hp->h_name);

	 /* argc-optind is the # of other servers on the command line */
	 serv_list = (char **)malloc((argc-optind + 2) * sizeof(char *));
	 serv_list[numserv++] = prim_serv;
	 for (; optind < argc; optind++) {
	     if ((hp = gethostbyname(argv[optind])) == NULL) {
		 printf("Unknown server name '%s', ignoring\n", argv[optind]);
		 continue;
	     }
	     serv_list[numserv++] = strsave(hp->h_name);
	 }
	 serv_list[numserv] = NULL;
     }
#ifdef ZEPHYR_USES_HESIOD
     else
	 ZEPHYR_USES_hesiod = 1;
#endif

     choose_server();
     if (*prim_serv == '\0') {
	 printf("No valid primary server found, exiting.\n");
	 exit(ZERR_SERVNAK);
     }
     init_hm();

     DPR2 ("zephyr server port: %u\n", ntohs(serv_sin.sin_port));
     DPR2 ("zephyr client port: %u\n", ntohs(cli_port));
  
     /* Main loop */
     for ever {
	  DPR ("Waiting for a packet...");
	  switch(sig_type) {
	  case 0:
	       break;
	  case SIGHUP:	/* A SIGHUP means we are deactivating the ws. */
	       sig_type = 0;
	       if (dieflag) {
		   die_gracefully();
	       } else {
		   choose_server();
		   send_flush_notice(HM_FLUSH);
		   deactivated = 1;
	       }
	       break;
	  case SIGTERM:
	       sig_type = 0;
	       die_gracefully();
	       break;
	  case SIGALRM:
	       sig_type = 0;
	       handle_timeout();
	       break;
	  default:
	       sig_type = 0;
	       syslog (LOG_WARNING, "Unknown system interrupt.");
	       break;
	  }
	  ret = ZReceivePacket(packet, &pak_len, &from);
	  if ((ret != ZERR_NONE) && (ret != EINTR)){
	       Zperr(ret);
	       com_err("hm", ret, "receiving notice");
	  } else if (ret != EINTR) {
	       /* Where did it come from? */
	       if ((ret = ZParseNotice(packet, pak_len, &notice))
		   != ZERR_NONE) {
		   Zperr(ret);
		   com_err("hm", ret, "parsing notice");
	      } else {
		   DPR ("Got a packet.\n");
		   DPR ("notice:\n");
		   DPR2("\tz_kind: %d\n", notice.z_kind);
		   DPR2("\tz_port: %u\n", ntohs(notice.z_port));
		   DPR2("\tz_class: %s\n", notice.z_class);
		   DPR2("\tz_class_inst: %s\n", notice.z_class_inst);
		   DPR2("\tz_opcode: %s\n", notice.z_opcode);
		   DPR2("\tz_sender: %s\n", notice.z_sender);
		   DPR2("\tz_recip: %s\n", notice.z_recipient);
		   DPR2("\tz_def_format: %s\n", notice.z_default_format);
		   DPR2("\tz_message: %s\n", notice.z_message);
		   if (memcmp(loopback, (char *)&from.sin_addr, 4) &&
		       ((notice.z_kind == SERVACK) ||
			(notice.z_kind == SERVNAK) ||
			(notice.z_kind == HMCTL))) {
		       server_manager(&notice);
		  } else {
		       if (!memcmp(loopback, (char *)&from.sin_addr, 4) &&
			   ((notice.z_kind == UNSAFE) ||
			    (notice.z_kind == UNACKED) ||
			    (notice.z_kind == ACKED) ||
			    (notice.z_kind == HMCTL))) {
			   /* Client program... */
			   if (deactivated) {
				send_boot_notice(HM_BOOT);
				deactivated = 0;
			   }
			   transmission_tower(&notice, packet, pak_len);
			   DPR2 ("Pending = %d\n", ZPending());
		      } else {
			   if (notice.z_kind == STAT) {
				send_stats(&notice, &from);
			   } else {
				syslog(LOG_INFO,
				       "Unknown notice type: %d",
				       notice.z_kind);
			   }
		      }
		  }
	      }
	  }
     }
}

static void choose_server()
{
    int i = 0;

#ifdef ZEPHYR_USES_HESIOD
    if (ZEPHYR_USES_hesiod) {

	/* Free up any previously used resources */
	if (prim_serv[0]) 
	    i = 1;
	while (i < numserv)
	    (void) free(serv_list[i++]);
	if (serv_list)
	    (void) free(serv_list);
	
	numserv = 0;
	prim_serv[0] = '\0';
	
	if ((clust_info = hes_resolve(hostname, "CLUSTER")) == NULL) {
	    zcluster = NULL;
	} else
	for ( ; *clust_info; clust_info++) {
	    /* Remove the following check once we have changed over to
	     * new Hesiod format (i.e. ZCLUSTER.sloc lookup, no primary
	     * server
	     */
	    if (!strncasecmp("ZEPHYR", *clust_info, 6)) {
		register char *c;
		
		if ((c = strchr(*clust_info, ' ')) == 0) {
		    printf("Hesiod error getting primary server info.\n");
		} else
		    (void)strcpy(prim_serv, c+1);
		break;
	    }
	    if (!strncasecmp("ZCLUSTER", *clust_info, 9)) {
		register char *c;
		
		if ((c = strchr(*clust_info, ' ')) == 0) {
		    printf("Hesiod error getting zcluster info.\n");
		} else {
		    if ((zcluster = malloc((unsigned)(strlen(c+1)+1)))
			!= NULL) {
			(void)strcpy(zcluster, c+1);
		    } else {
			printf("Out of memory.\n");
			exit(-5);
		    }
		}
		break;
	    }
	}
	
	if (zcluster == NULL) {
	    if ((zcluster = malloc((unsigned)(strlen("zephyr")+1))) != NULL)
		(void)strcpy(zcluster, "zephyr");
	}
	while ((serv_list = hes_resolve(zcluster, "sloc")) == (char **)NULL) {
	    syslog(LOG_ERR, "No servers or no hesiod");
	    /* wait a bit, and try again */
	    sleep(30);
	}
	clust_info = (char **)malloc(2*sizeof(char *));
	if (prim_serv[0])
	    clust_info[numserv++] = prim_serv;
	for (i = 0; serv_list[i]; i++)
	    /* copy in non-duplicates */
	    /* assume the names returned in the sloc are full domain names */
	if (!prim_serv[0] || strcasecmp(prim_serv, serv_list[i])) {
	    clust_info = (char **) realloc(clust_info,
					   (numserv+2) * sizeof(char *));
	    clust_info[numserv++] = strsave(serv_list[i]);
	}
	clust_info[numserv] = NULL;
	serv_list = clust_info;
    }
#endif
    
    if (!prim_serv[0] && numserv) {
	srandom(time(NULL));
	(void) strcpy(prim_serv, serv_list[random() % numserv]);
    }
}

static void init_hm()
{
     struct servent *sp;
     Code_t ret;
     FILE *fp;
#ifdef _POSIX_VERSION
     struct sigaction sa;
#endif

     starttime = time((time_t *)0);
     OPENLOG("hm", LOG_PID, LOG_DAEMON);
  
     if ((ret = ZInitialize()) != ZERR_NONE) {
	  Zperr(ret);
	  com_err("hm", ret, "initializing");
	  closelog();
	  exit(-1);
     }
     (void)ZSetServerState(1);	/* Aargh!!! */
     init_queue();

     if (*prim_serv == '\0')
	  (void)strcpy(prim_serv, *serv_list);
  
     loopback[0] = 127;
     loopback[1] = 0;
     loopback[2] = 0;
     loopback[3] = 1;
      
     if (inetd) {
	     (void) ZSetFD(0);		/* fd 0 is on the socket,
					   thanks to inetd */
     } else {
	     /* Open client socket, for receiving client and server notices */
	     sp = getservbyname(HM_SVCNAME, "udp");
	     cli_port = (sp) ? sp->s_port : HM_SVC_FALLBACK;
      
	     if ((ret = ZOpenPort(&cli_port)) != ZERR_NONE) {
		     Zperr(ret);
		     com_err("hm", ret, "opening port");
		     exit(ret);
	     }
     }
     cli_sin = ZGetDestAddr();

     sp = getservbyname(SERVER_SVCNAME, "udp");
     (void) memset((char *)&serv_sin, 0, sizeof(struct sockaddr_in));
     serv_sin.sin_port = (sp) ? sp->s_port : SERVER_SVC_FALLBACK;
      

#ifndef DEBUG
     if (!inetd && !nofork)
	     detach();
  
     /* Write pid to file */
     fp = fopen(PidFile, "w");
     if (fp != NULL) {
	     fprintf(fp, "%d\n", getpid());
	     (void) fclose(fp);
     }
#endif /* DEBUG */

     if (hmdebug) {
	  syslog(LOG_INFO, "Debugging on.");
     }

     /* Set up communications with server */
     /* target is SERVER_SVCNAME port on server machine */

     serv_sin.sin_family = AF_INET;
  
     /* who to talk to */
     if ((hp = gethostbyname(prim_serv)) == NULL) {
	  DPR("gethostbyname failed\n");
	  find_next_server((char *)NULL);
     } else {
	  DPR2 ("Server = %s\n", prim_serv);
	  (void)strcpy(cur_serv, prim_serv);
	  (void)memcpy((char *)&serv_sin.sin_addr, hp->h_addr, hp->h_length);
     }

     send_boot_notice(HM_BOOT);
     deactivated = 0;

#ifdef _POSIX_VERSION
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = 0;
     sa.sa_handler = set_sig_type;
     sigaction(SIGHUP, &sa, (struct sigaction *)0);
     sigaction(SIGALRM, &sa, (struct sigaction *)0);
     sigaction(SIGTERM, &sa, (struct sigaction *)0);
#else
     (void)signal (SIGHUP,  set_sig_type);
     (void)signal (SIGALRM, set_sig_type);
     (void)signal (SIGTERM, set_sig_type);
#endif
}

static void detach()
{
     /* detach from terminal and fork. */
     register int i, x = ZGetFD();
     register long size;
  
     if (i = fork()) {
	  if (i < 0)
	       perror("fork");
	  exit(0);
     }
#ifdef _POSIX_VERSION
     size = sysconf(_SC_OPEN_MAX);
#else
     size = getdtablesize();
#endif
     for (i = 0; i < size; i++)
	  if (i != x)
	       (void) close(i);

     if ((i = open("/dev/tty", O_RDWR, 666)) < 0)
	  ;		/* Can't open tty, but don't flame about it. */
     else {
#ifdef TIOCNOTTY
	  /* Necessary for old non-POSIX systems which automatically assign
	   * an opened tty as the controlling terminal of a process which
	   * doesn't already have one.  POSIX systems won't include
	   * <sys/ioctl.h> (see ../h/sysdep.h); if TIOCNOTTY is defined anyway,
	   * this is unnecessary but won't hurt. */
	  (void) ioctl(i, TIOCNOTTY, (caddr_t) 0);
#endif
	  (void) close(i);
     }
#ifdef _POSIX_VERSION
     (void) setsid();
#endif
}

static char version[BUFSIZ];

static void send_stats(notice, sin)
     ZNotice_t *notice;
     struct sockaddr_in *sin;
{
     ZNotice_t newnotice;
     Code_t ret;
     char *bfr;
     char *list[20];
     int len, i, nitems = 10;
     unsigned long size;

     newnotice = *notice;
     
     if ((ret = ZSetDestAddr(sin)) != ZERR_NONE) {
	  Zperr(ret);
	  com_err("hm", ret, "setting destination");
     }
     newnotice.z_kind = HMACK;

     list[0] = (char *)malloc(MAXHOSTNAMELEN);
     (void)strcpy(list[0], cur_serv);
     list[1] = (char *)malloc(64);
     (void)sprintf(list[1], "%d", queue_len());
     list[2] = (char *)malloc(64);
     (void)sprintf(list[2], "%d", nclt);
     list[3] = (char *)malloc(64);
     (void)sprintf(list[3], "%d", nserv);
     list[4] = (char *)malloc(64);
     (void)sprintf(list[4], "%d", nservchang);
     list[5] = (char *)malloc(64);
     (void)strcpy(list[5], rcsid_hm_c);
     list[6] = (char *)malloc(64);
     if (no_server)
	  (void)sprintf(list[6], "yes");
     else
	  (void)sprintf(list[6], "no");
     list[7] = (char *)malloc(64);
     (void)sprintf(list[7], "%ld", time((time_t *)0) - starttime);
#ifdef adjust_size
     size = (unsigned long)sbrk(0);
     adjust_size (size);
#else
     size = -1;
#endif
     list[8] = (char *)malloc(64);
     (void)sprintf(list[8], "%ld", size);
     list[9] = (char *)malloc(32);
     (void)strcpy(list[9], MACHINE_TYPE);

     /* Since ZFormatRaw* won't change the version number on notices,
	we need to set the version number explicitly.  This code is taken
	from Zinternal.c, function Z_FormatHeader */
     if (!*version)
	     (void) sprintf(version, "%s%d.%d", ZVERSIONHDR, ZVERSIONMAJOR,
			    ZVERSIONMINOR);
     newnotice.z_version = version;

     if ((ret = ZFormatRawNoticeList(&newnotice, list, nitems, &bfr,
				     &len)) != ZERR_NONE) {
	  syslog(LOG_INFO, "Couldn't format stats packet");
     } else
	  if ((ret = ZSendPacket(bfr, len, 0)) != ZERR_NONE) {
	       Zperr(ret);
	       com_err("hm", ret, "sending stats");
	  }
     free(bfr);
     for(i=0;i<nitems;i++)
	  free(list[i]);
}

static void handle_timeout()
{
     switch(timeout_type) {
     case BOOTING:
	  new_server((char *)NULL);
	  break;
     case NOTICES:
	  DPR ("Notice timeout\n");
	  resend_notices(&serv_sin);
	  break;
     default:
	  syslog (LOG_ERR, "Unknown timeout type: %d\n", timeout_type);
	  break;
     }
}

void die_gracefully()
{
     syslog(LOG_INFO, "Terminate signal caught...");
     send_flush_notice(HM_FLUSH);
     (void)unlink(PidFile);
     closelog();
     exit(0);
}

static char *strsave(sp)
    const char *sp;
{
    register char *ret;

    if((ret = malloc((unsigned) strlen(sp)+1)) == NULL) {
	    abort();
    }
    (void) strcpy(ret,sp);
    return(ret);
}
