/* This file is part of the Project Athena Zephyr Notification System.
 * It contains code for the "zwrite" command.
 *
 *	Created by:	Robert French
 *
 *	$Id$
 *
 *	Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <sysdep.h>
#include <zephyr/mit-copyright.h>
#include <zephyr/zephyr.h>
#include <netdb.h>
#include <pwd.h>

#ifndef lint
static const char rcsid_zwrite_c[] = "$Id$";
#endif /* lint */

#define DEFAULT_CLASS "MESSAGE"
#define DEFAULT_INSTANCE "PERSONAL"
#define URGENT_INSTANCE "URGENT"
#define DEFAULT_OPCODE ""
#define FILSRV_CLASS "FILSRV"

#define MAXRECIPS 100

int nrecips, msgarg, verbose, quiet, nodot, cc;
char *whoami, *inst, *class, *opcode, *realm, *recips[MAXRECIPS];
Z_AuthProc auth;
void un_tabify(char **, int *);

char *fix_filsrv_inst(char *);
void usage(char *);
void send_off(ZNotice_t *, int);

int
main(int argc, char *argv[])
{
    int retval, arg, nocheck, nchars, msgsize, filsys, tabexpand;
    char *message, *signature = NULL, *format = NULL;
    static char bfr[BUFSIZ], classbfr[BUFSIZ], instbfr[BUFSIZ], sigbfr[BUFSIZ];
    static char opbfr[BUFSIZ];
    static ZNotice_t notice;
    char *charset = NULL;

    whoami = argv[0];

    if ((retval = ZInitialize()) != ZERR_NONE) {
	com_err(whoami, retval, "while initializing");
	exit(1);
    }

    if (argc < 2)
	usage(whoami);

    auth = ZAUTH;
    verbose = quiet = msgarg = nrecips = nocheck = filsys = nodot = 0;
    tabexpand = 1;

    class = ZGetVariable("zwrite-class");
    if (class) {
	(void) strcpy(classbfr, class);
	class = classbfr;
    }
    else
	class = DEFAULT_CLASS;
    inst = ZGetVariable("zwrite-inst");
    if (inst) {
	(void) strcpy(instbfr, inst);
	inst = instbfr;
    }
    else
	inst = DEFAULT_INSTANCE;

    opcode = ZGetVariable("zwrite-opcode");
    if (opcode)
      opcode = strcpy(opbfr, opcode);
    else
      opcode = DEFAULT_OPCODE;

      signature = ZGetVariable("zwrite-signature");
    if (signature) {
	(void) strcpy(sigbfr, signature);
	signature = sigbfr;
    } 
	
    arg = 1;
	
    for (;arg<argc && !msgarg; arg++) {
	if (*argv[arg] != '-') {
	    recips[nrecips++] = argv[arg];
	    continue;
	} 
	if (strlen(argv[arg]) > 2)
	    usage(whoami);
	switch (argv[arg][1]) {
	case 'a':		/* Backwards compatibility */
	    auth = ZAUTH;
	    break;
	case 'o':
	    class = DEFAULT_CLASS;
	    inst = DEFAULT_INSTANCE;
	    opcode = DEFAULT_OPCODE;
	    break;
	case 'd':
	    auth = ZNOAUTH;
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'q':
	    quiet = 1;
	    break;
	case 'n':
	    nocheck = 1;
	    break;
	case 't':
	    tabexpand = 0;
	    break;
	case 'u':
	    inst = URGENT_INSTANCE;
	    break;
	case 'O':
	    if (arg == argc-1)
	      usage(whoami);
	    arg++;
	    opcode = argv[arg];
	    break;
	case 'i':
	    if (arg == argc-1 || filsys == 1)
		usage(whoami);
	    arg++;
	    inst = argv[arg];
	    filsys = -1;
	    break;
	case 'c':
	    if (arg == argc-1 || filsys == 1)
		usage(whoami);
	    arg++;
	    class = argv[arg];
	    filsys = -1;
	    break;
	case 'f':
	    if (arg == argc-1 || filsys == -1)
		usage(whoami);
	    arg++;
	    class = FILSRV_CLASS;
	    inst = fix_filsrv_inst(argv[arg]);
	    filsys = 1;
	    nocheck = 1;		/* implied -n (no ping) */
	    break;
	case 's':
	    if (arg == argc-1)
		usage(whoami);
	    arg++;
	    signature = argv[arg];
	    break;
	case 'm':
	    if (arg == argc-1)
		usage(whoami);
	    nocheck = 1;		/* implied -n (no ping) */
	    msgarg = arg+1;
	    break;
	case 'l':			/* literal */
	    nodot = 1;
	    break;
	case 'F':
	    if (arg == argc-1)
		usage(whoami);
	    arg++;
	    format = argv[arg];
	    break;
	case 'r':
	    if (arg == argc-1)
		usage(whoami);
	    arg++;
	    realm = argv[arg];
	    break;
	case 'C':
	    cc = 1;
	    break;
	case 'x':
	    if (arg == argc-1)
		usage(whoami);
	    arg++;
	    charset = argv[arg];
	    break;
	default:
	    usage(whoami);
	}
    }

    if (!nrecips && !(strcmp(class, DEFAULT_CLASS) ||
		      (strcmp(inst, DEFAULT_INSTANCE) &&
		       strcmp(inst, URGENT_INSTANCE)))) {
	/* must specify recipient if using default class and
	   (default instance or urgent instance) */
	fprintf(stderr, "No recipients specified.\n");
	usage(whoami);
    }

    if (!signature) {
	/* try to find name in the password file */
	register struct passwd *pwd;
	register char *cp = sigbfr;
	register char *cp2, *pp;

	pwd = getpwuid(getuid());
	if (pwd) {
	    cp2 = pwd->pw_gecos;
	    for (; *cp2 && *cp2 != ',' ; cp2++) {
		if (*cp2 == '&') {
		    pp = pwd->pw_name;
		    *cp++ = islower(*pp) ? toupper(*pp) : *pp;
		    pp++;
		    while (*pp)
			*cp++ = *pp++;
		} else
		    *cp++ = *cp2;
	    }
	    signature = sigbfr;
	}
    }	

    notice.z_kind = ACKED;
    notice.z_port = 0;
    notice.z_class = class;
    notice.z_class_inst = inst;
    notice.z_opcode = "PING";
    notice.z_sender = 0;
    notice.z_message_len = 0;
    notice.z_recipient = "";
    notice.z_charset = ZGetCharset(charset);
    if (format)
	    notice.z_default_format = format;
    else if (filsys == 1)
	    notice.z_default_format = "@bold(Filesystem Operation Message for $instance:)\nFrom: @bold($sender) at $time $date\n$message";
    else if (auth == ZAUTH) {
	if (signature)
	    notice.z_default_format = "Class $class, Instance $instance:\nTo: @bold($recipient) at $time $date\nFrom: @bold($1) <$sender>\n\n$2";
	else
	    notice.z_default_format = "Class $class, Instance $instance:\nTo: @bold($recipient) at $time $date\n$message";
    } else {
	if (signature)
	    notice.z_default_format = "@bold(UNAUTHENTIC) Class $class, Instance $instance at $time $date:\nFrom: @bold($1) <$sender>\n\n$2";
	else
	    notice.z_default_format = "@bold(UNAUTHENTIC) Class $class, Instance $instance at $time $date:\n$message";
    }
    if (!nocheck && nrecips)
	send_off(&notice, 0);
	
    if (!msgarg && isatty(0)) {
	if (nodot)
	    printf("Type your message now.  End with the end-of-file character.\n");
	else
	    printf("Type your message now.  End with control-D or a dot on a line by itself.\n");
    }
	
    message = NULL;
    msgsize = 0;
    if (signature) {
	message = malloc((unsigned)(strlen(signature)+2));
	(void) strcpy(message, signature);
	msgsize = strlen(message);
	message[msgsize++] = '\0';
    } else {
	message = malloc(1);
	message[msgsize++] = '\0';
    }

    if (cc && nrecips > 1) {
        int size = msgsize;
    	for (arg=0;arg<nrecips;arg++)
	    size += (strlen(recips[arg]) + 2);
	size += 6;			/* for the newlines and "cc: " */
	message = realloc(message, (unsigned) size);
	(void) strcpy(message+msgsize, "CC: ");
	msgsize += 4;
	for (arg=0;arg<nrecips;arg++) {
	    (void) strcpy(message+msgsize, recips[arg]);
	    msgsize += strlen(recips[arg]);
	    if (arg != nrecips-1) {
		message[msgsize] = ' ';
		msgsize++;
	    }
	}
	message[msgsize] = '\n';
	msgsize += 1;
    }

    if (msgarg) {
	int size = msgsize;
	for (arg=msgarg;arg<argc;arg++)
		size += (strlen(argv[arg]) + 1);
	size++;				/* for the newline */
	message = realloc(message, (unsigned) size);
	for (arg=msgarg;arg<argc;arg++) {
	    (void) strcpy(message+msgsize, argv[arg]);
	    msgsize += strlen(argv[arg]);
	    if (arg != argc-1) {
		message[msgsize] = ' ';
		msgsize++;
	    } 
	}
	message[msgsize] = '\n';
	msgsize += 1;
    } else {
	if (isatty(0)) {
	    for (;;) {
		unsigned int l;
		if (!fgets(bfr, sizeof bfr, stdin))
		    break;
		if (!nodot && bfr[0] == '.' &&
		    (bfr[1] == '\n' || bfr[1] == '\0'))
		    break;
		l = strlen(bfr);
		message = realloc(message, msgsize+l+1);
		(void) strcpy(message+msgsize, bfr);
		msgsize += l;
	    }
	    message = realloc(message, (unsigned)(msgsize+1));
	}
	else {	/* Use read so you can send binary messages... */
	    while ((nchars = read(fileno(stdin), bfr, sizeof bfr))) {
		if (nchars == -1) {
		    fprintf(stderr, "Read error from stdin!  Can't continue!\n");
		    exit(1);
		}
		message = realloc(message, (unsigned)(msgsize+nchars));
		(void) memcpy(message+msgsize, bfr, nchars);
		msgsize += nchars;
	    }
	    /* end of msg */
	    message = realloc(message, (unsigned)(msgsize+1));
	} 
    }

    notice.z_opcode = opcode;
    if (tabexpand)
	un_tabify(&message, &msgsize);
    notice.z_message = message;
    notice.z_message_len = msgsize;

    send_off(&notice, 1);
    exit(0);
}

void
send_off(ZNotice_t *notice, int real)
{
    int i, success, retval;
    char bfr[BUFSIZ], realm_recip[BUFSIZ], dest[3 * BUFSIZ];
    ZNotice_t retnotice;

    success = 0;
	
    for (i=0;i<nrecips || i==0;i++) {
	if (realm) {
	    sprintf(realm_recip, "%s@%s", (nrecips) ? recips[i] : "", realm);
	    notice->z_recipient = realm_recip;
	} else {
	    notice->z_recipient = (nrecips) ? recips[i] : "";
	}
	if (nrecips)
	    strcpy(dest, recips[i]);
	else if (!strcmp(class, DEFAULT_CLASS))
	    sprintf(dest, "instance \"%s\"", inst);
	else if (!strcmp(inst, DEFAULT_INSTANCE))
	    sprintf(dest, "class \"%s\"", class);
	else
	    sprintf(dest, "class \"%s\", instance \"%s\"", class, inst);
	if (verbose && real)
	    printf("Sending %smessage, class %s, instance %s, to %s\n", 
		   auth?"authenticated ":"", 
		   class, inst, 
		   nrecips?notice->z_recipient:"everyone");
	if ((retval = ZSendNotice(notice, auth)) != ZERR_NONE) {
	    (void) sprintf(bfr, "while sending notice to %s", dest);
	    com_err(whoami, retval, bfr);
	    break;
	}
	if (real && !quiet) {
	    if (verbose)
		printf("Queued... ");
	    else
		printf("Message queued for %s... ", dest);
	    fflush(stdout);
	}
	if ((retval = ZIfNotice(&retnotice, (struct sockaddr_in *) 0,
				ZCompareUIDPred, 
				(char *)&notice->z_uid)) !=
	    ZERR_NONE) {
	    ZFreeNotice(&retnotice);
	    (void) sprintf(bfr, "while waiting for acknowledgement for %s", 
		    dest);
	    com_err(whoami, retval, bfr);
	    continue;
	}
	if (retnotice.z_kind == SERVNAK) {
	    if (!quiet) {
		printf("Received authorization failure while sending to %s\n", 
		       dest);
	    }
	    ZFreeNotice(&retnotice);
	    break;			/* if auth fails, punt */
	} 
	if (retnotice.z_kind != SERVACK || !retnotice.z_message_len) {
	    if (!quiet) {
		printf("Detected server failure while receiving acknowledgement for %s\n", 
		       dest);
	    }
	    ZFreeNotice(&retnotice);
	    continue;
	}
	if (!strcmp(retnotice.z_message, ZSRVACK_SENT)) {
	    success = 1;
	    if (real && !quiet)
		printf("sent\n");
	} else if (!strcmp(retnotice.z_message, ZSRVACK_NOTSENT)) {
	    if (verbose && real && !quiet) {
		if (strcmp(class, DEFAULT_CLASS)) {
		    fprintf(stderr, "Not logged in or not subscribing to class %s, instance %s\n", 
			   class, inst);
		} else {
		    fprintf(stderr,
			    "Not logged in or not subscribing to messages\n");
		}
	    } 
	    else if (!quiet) {
		if (!nrecips) {
		    fprintf(stderr,
			    "No one subscribing to class %s, instance %s\n", 
			    class, inst);
		} else {
		    if (strcmp(class, DEFAULT_CLASS)) {
			fprintf(stderr, "%s: Not logged in or not subscribing to class %s, instance %s\n", 
			       notice->z_recipient, class, inst);
		    } else {
			fprintf(stderr, "%s: Not logged in or not subscribing to messages\n", 
			       notice->z_recipient);
		    }
		}
	    }
	} 
	else
	    printf("Internal failure - illegal message field in server response\n");
	ZFreeNotice(&retnotice);
    }
    if (!success)
	exit(1);
} 

void
usage(char *s)
{
    fprintf(stderr,
	    "Usage: %s [-a] [-o] [-d] [-v] [-q] [-n] [-t] [-u] [-l]\n\
\t[-c class] [-i inst] [-O opcode] [-f fsname] [-s signature] [-C]\n\
\t[user ...] [-F format] [-r realm] [-x charset] [-m message]\n", s);
    fprintf(stderr,"\t-f and -c are mutually exclusive\n\
\t-f and -i are mutually exclusive\n\
\trecipients must be specified unless -c or -f specifies a class\n\
\tother than the default class or -i or -f specifies an instance\n\
\tother than the default or urgent instance\n");
    exit(1);
} 

/*
  if the -f option is specified, this routine is called to canonicalize
  an instance of the form hostname[:pack].  It turns the hostname into the
  name returned by gethostbyname(hostname)
 */

char *
fix_filsrv_inst(char *str)
{
	static char fsinst[BUFSIZ];
	char *ptr;
	struct hostent *hp;

	ptr = strchr(str,':');
	if (ptr)
		*ptr = '\0';
	
	hp = gethostbyname(str);
	if (!hp) {
		if (ptr)
			*ptr = ':';
		return(str);
	}
	(void) strcpy(fsinst, hp->h_name);
	if (ptr) {
		(void) strcat(fsinst, ":");
		ptr++;
		(void) strcat(fsinst, ptr);
	}
	return(fsinst);
}

/* convert tabs in the buffer into appropriate # of spaces.
   slightly tricky since the buffer can have NUL's in it. */

#ifndef TABSTOP
#define	TABSTOP	8			/* #chars between tabstops */
#endif /* ! TABSTOP */

void
un_tabify(char **bufp,
	  int *sizep)
{
    register char *cp, *cp2;
    char *cp3;
    register int i;
    register int column;		/* column of next character */
    register int size = *sizep;

    for (cp = *bufp, i = 0; size; size--, cp++)
	if (*cp == '\t')
	    i++;			/* count tabs in buffer */

    if (!i)
	return;				/* no tabs == no work */

    /* To avoid allocation churning, allocate enough extra space to convert
       every tab into TABSTOP spaces */
    /* only add (TABSTOP-1)x because we re-use the cell holding the
       tab itself */
    cp = malloc((unsigned)(*sizep + (i * (TABSTOP-1))));
    if (!cp)				/* XXX */
	return;				/* punt expanding if memory fails */
    cp3 = cp;
    /* Copy buffer, converting tabs to spaces as we go */
    for (cp2 = *bufp, column = 1, size = *sizep; size; cp2++, size--) {
	switch (*cp2) {
	case '\n':
	case '\0':
	    /* newline or null: reset column */
	    column = 1;
	    *cp++ = *cp2;		/* copy the newline */
	    break;
	default:
	    /* copy the character */
	    *cp = *cp2;
	    cp++;
	    column++;
	    break;
	case '\t':
	    /* it's a tab, compute how many spaces to expand into. */
	    i = TABSTOP - ((column - 1) % TABSTOP);
	    for (; i > 0; i--) {
		*cp++ = ' ';		/* fill in the spaces */
		column++;
		(*sizep)++;		/* increment the size */
	    }
	    (*sizep)--;			/* remove one (we replaced the tab) */
	    break;
	}
    }
    free(*bufp);			/* free the old buf */
    *bufp = cp3;
    return;
}
