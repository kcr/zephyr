/* This file is part of the Project Athena Zephyr Notification System.
 * It contains code for the "zlocate" command.
 *
 *	Created by:	Robert French
 *
 *	$Id$
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <sysdep.h>
#include <zephyr/zephyr.h>
#include <sys/socket.h>

#if !defined(lint) && !defined(SABER)
static const char rcsid_zlocate_c[] = "$Id$";
#endif

int numusers=0, numleft=0, parallel=0, oneline=0;
char *whoami;

RETSIGTYPE
timeout(int sig)
{
  fprintf (stderr, "%s: no response from server\n", whoami);
  exit(1);
}

void
usage(void)
{
   printf("Usage: %s [ -a | -d ] [ -p ] [ -1 ] user ... \n",whoami);
   exit(1);
}

void
print_locs(char *user,
	   int nlocs)
{
   int one = 1, retval;
   ZLocations_t locations;

   if ((!oneline) && (numusers>1))
     printf("\t%s:\n",user);

   if ((!oneline) && (nlocs == 0))
      printf("Hidden or not logged-in\n");

   for (;nlocs;nlocs--) {
      if ((retval = ZGetLocations(&locations,&one)) != ZERR_NONE) {
	 com_err(whoami,retval,"while getting location");
	 exit(1);
      }
 
      if (oneline) {
	 printf("%s:\t%s\t%s\t%s\n",user,locations.host,locations.tty,
		locations.time);
      } else {
	 printf("%-42s %-7s %s\n",locations.host, locations.tty, locations.time);
      }
   }

   if ((!oneline) && (numusers > 1) && (numleft > 0))
     printf("\n");
}

/*ARGSUSED*/
int
main(int argc,
     char *argv[])
{
    char user[BUFSIZ],*whichuser;
    ZAsyncLocateData_t ald;
    int retval,i,numlocs,numfound,loc,auth,rlen;
    ZNotice_t notice;
#ifdef _POSIX_VERSION
    struct sigaction sa;
#endif
   
    whoami = argv[0];
    auth = -1;

    argv++;
    argc--;

    for (i=0; i < argc; i++)
	if (argv[i][0] == '-')
	    switch (argv[i][1]) {
	    case 'a':
		if (auth != -1) usage();
		auth = 1;
		break;
	    case 'd':
		if (auth != -1) usage();
		auth = 0;
		break;
	    case 'p':
		parallel = 1;
		break;
	    case '1':
		oneline = 1;
		break;
	    default:
		usage();
		break;
	    }
	else
	    numusers++;

    if (numusers == 0)
	usage();

    if (auth == -1) auth = 1;
   
    if ((retval = ZInitialize()) != ZERR_NONE) {
	com_err(whoami,retval,"while initializing");
	exit(1);
    } 

    numleft = numusers;
    numfound = 0;
    rlen = strlen(ZGetRealm());

    i = 0;
    for (loc = 0; loc < argc; loc++) {
	if (argv[loc][0] == '-') continue;

	(void) strncpy(user,argv[loc],sizeof(user) - rlen - 2);
	user[sizeof(user) - rlen - 2] = '\0';
	if (!strchr(user,'@')) {
	    (void) strcat(user,"@");
	    (void) strcat(user,ZGetRealm());
	} 
	if (parallel) {
	    if ((retval = ZRequestLocations(user, &ald, i ? UNSAFE : UNACKED,
					    auth?ZAUTH:ZNOAUTH)) != ZERR_NONE) {
		com_err(whoami,retval,"requesting location of %s",user);
		exit(1);
	    }
	    i = 1;
	} else {
	    if ((retval = ZLocateUser(user,&numlocs,auth?ZAUTH:ZNOAUTH)) != ZERR_NONE) {
		com_err(whoami,retval,"while locating user %s",user);
		exit(1);
	    }
	    print_locs(user,numlocs);
	    numfound += numlocs;
	}
    }

    if (parallel) {
#ifdef _POSIX_VERSION
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = timeout;
	sigaction(SIGALRM, &sa, (struct sigaction *)0);
#else
	signal (SIGALRM, timeout);
#endif
	while (numleft-- > 0) {
	    alarm(SRV_TIMEOUT);
	    if ((retval = ZReceiveNotice(&notice, NULL)) != ZERR_NONE) {
		com_err(whoami,retval,"while searching notice queue");
		continue;
	    }
	    if ((retval = ZParseLocations(&notice, (ZAsyncLocateData_t *)NULL,
					  &numlocs, &whichuser)) != ZERR_NONE) {
		com_err(whoami,retval,"while parsing locations");
		continue;
	    }
	    if (numlocs >= 0) {
		print_locs(whichuser,numlocs);
		free(whichuser);
		numfound += numlocs;
	    }
	    ZFreeNotice(&notice);
	}
    }
    return((numfound > 0) ? 0 : 1);
}
