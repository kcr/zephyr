/* This file is part of the Project Athena Zephyr Notification System.
 * It contains code for the "zwrite" command.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/clients/zwrite/zwrite.c,v $
 *	$Author: jtkohl $
 *
 *	Copyright (c) 1987,1988 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr.h>
#include <string.h>
#include <netdb.h>

#ifndef lint
static char rcsid_zwrite_c[] = "$Header: /srv/kcr/locker/zephyr/clients/zwrite/zwrite.c,v 1.25 1988-11-14 11:58:28 jtkohl Exp $";
#endif lint

#define DEFAULT_CLASS "MESSAGE"
#define DEFAULT_INSTANCE "PERSONAL"
#define URGENT_INSTANCE "URGENT"
#define FILSRV_CLASS "FILSRV"

#define MAXRECIPS 100

int nrecips, msgarg, verbose, quiet;
char *whoami, *inst, *class, *recips[MAXRECIPS];
int (*auth)();
void un_tabify();

extern char *malloc(), *realloc();
char *fix_filsrv_inst();

main(argc, argv)
    int argc;
    char *argv[];
{
    ZNotice_t notice;
    int retval, arg, nocheck, nchars, msgsize, filsys, tabexpand;
    char bfr[BUFSIZ], *message, *signature;
    char classbfr[BUFSIZ], instbfr[BUFSIZ], sigbfr[BUFSIZ];
	
    whoami = argv[0];

    if ((retval = ZInitialize()) != ZERR_NONE) {
	com_err(whoami, retval, "while initializing");
	exit(1);
    } 

    if (argc < 2)
	usage(whoami);

    bzero((char *) &notice, sizeof(notice));

    auth = ZAUTH;
    verbose = quiet = msgarg = nrecips = nocheck = filsys = 0;
    tabexpand = 1;

    if (class = ZGetVariable("zwrite-class")) {
	(void) strcpy(classbfr, class);
	class = classbfr;
    }
    else
	class = DEFAULT_CLASS;
    if (inst = ZGetVariable("zwrite-inst")) {
	(void) strcpy(instbfr, inst);
	inst = instbfr;
    }
    else
	inst = DEFAULT_INSTANCE;
    signature = ZGetVariable("zwrite-signature");
    if (signature) {
	(void) strcpy(sigbfr, signature);
	signature = sigbfr;
    } 
	
    arg = 1;
	
    for (;arg<argc&&!msgarg;arg++) {
	if (*argv[arg] != '-') {
	    recips[nrecips++] = argv[arg];
	    continue;
	} 
	if (strlen(argv[arg]) > 2)
	    usage(whoami);
	switch (argv[arg][1]) {
	case 'a':		/* Backwards compatibility */
	    break;
	case 'o':
	    class = DEFAULT_CLASS;
	    inst = DEFAULT_INSTANCE;
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
	    break;
	case 'm':
	    if (arg == argc-1)
		usage(whoami);
	    msgarg = arg+1;
	    break;
	default:
	    usage(whoami);
	}
    }

    if (!nrecips && !(strcmp(class, DEFAULT_CLASS) ||
		      strcmp(inst, DEFAULT_INSTANCE))) {
	fprintf(stderr, "No recipients specified.\n");
	exit (1);
    }

    notice.z_kind = ACKED;
    notice.z_port = 0;
    notice.z_class = class;
    notice.z_class_inst = inst;
    notice.z_opcode = "PING";
    notice.z_sender = 0;
    notice.z_message_len = 0;
    notice.z_recipient = "";
    if (filsys == 1)
	    notice.z_default_format = "@bold(Filesystem Operation Message for $instance:)\nFrom: @bold($sender)\n$message";
    else if (auth == ZAUTH) {
	if (signature)
	    notice.z_default_format = "Class $class, Instance $instance:\nTo: @bold($recipient)\n@bold($1) <$sender>\n\n$2";
	else
	    notice.z_default_format = "Class $class, Instance $instance:\nTo: @bold($recipient)\n$message";
    } else {
	if (signature)
	    notice.z_default_format = "@bold(UNAUTHENTIC) Class $class, Instance $instance:\n@bold($1) <$sender>\n\n$2";
	else
	    notice.z_default_format = "@bold(UNAUTHENTIC) Class $class, Instance $instance:\n$message";
    }
    if (!nocheck && !msgarg && filsys != 1)
	send_off(&notice, 0);
	
    if (!msgarg && isatty(0))
	printf("Type your message now.  End with control-D or a dot on a line by itself.\n");

    message = NULL;
    msgsize = 0;
    if (signature) {
	message = malloc((unsigned)(strlen(signature)+sizeof("From: ")+2));
	(void) strcpy(message, "From: ");
	(void) strcat(message, signature);
	msgsize = strlen(message);
	message[msgsize++] = '\n';
	message[msgsize++] = '\0';
    }
	
    if (msgarg) {
	int size = msgsize;
	for (arg=msgarg;arg<argc;arg++)
		size += (strlen(argv[arg]) + 1);
	size++;				/* for the newline */
	if (message)
		message = realloc(message, (unsigned) size);
	else
		message = malloc((unsigned) size);
	for (arg=msgarg;arg<argc;arg++) {
	    (void) strcpy(message+msgsize, argv[arg]);
	    msgsize += strlen(argv[arg]);
	    if (arg != argc-1) {
		message[msgsize] = ' ';
		msgsize++;
	    } 
	}
	message[msgsize] = '\n';
	message[msgsize+1] = '\0';
	msgsize += 2;
    } else {
	if (isatty(0)) {
	    for (;;) {
		if (!fgets(bfr, sizeof bfr, stdin))
		    break;
		if (bfr[0] == '.' &&
		    (bfr[1] == '\n' || bfr[1] == '\0'))
		    break;
		if (message)
			message = realloc(message,
					  (unsigned)(msgsize+strlen(bfr)));
		else
			message = malloc((unsigned)strlen(bfr));
		(void) strcpy(message+msgsize, bfr);
		msgsize += strlen(bfr);
	    }
	    message = realloc(message, (unsigned)(msgsize+1));
	    message[msgsize++] = '\0';
	}
	else {	/* Use read so you can send binary messages... */
	    while (nchars = read(fileno(stdin), bfr, sizeof bfr)) {
		if (nchars == -1) {
		    fprintf(stderr, "Read error from stdin!  Can't continue!\n");
		    exit(1);
		}
		if (message)
		    message = realloc(message, (unsigned)(msgsize+nchars));
		else
		    message = malloc((unsigned)nchars);
		bcopy(bfr, message+msgsize, nchars);
		msgsize += nchars;
	    }
	    /* end of msg */
	    if (message)
		message = realloc(message, (unsigned)(msgsize+1));
	    else
		message = malloc((unsigned) 1);
	    message[msgsize++] = '\0';	/* null-terminate */
	} 
    }

    notice.z_opcode = "";
    if (tabexpand)
	un_tabify(&message, &msgsize);
    notice.z_message = message;
    notice.z_message_len = msgsize;

    send_off(&notice, 1);
    exit(0);
}

send_off(notice, real)
    ZNotice_t *notice;
    int real;
{
    int i, success, retval;
    char bfr[BUFSIZ];
    ZNotice_t retnotice;

    success = 0;
	
    for (i=0;i<nrecips || !nrecips;i++) {
	notice->z_recipient = nrecips?recips[i]:"";
	if (verbose && real)
	    printf("Sending %smessage, class %s, instance %s, to %s\n", 
		   auth?"authenticated ":"", 
		   class, inst, 
		   nrecips?notice->z_recipient:"everyone");
	if ((retval = ZSendNotice(notice, auth)) != ZERR_NONE) {
	    (void) sprintf(bfr, "while sending notice to %s", 
		    nrecips?notice->z_recipient:inst);
	    com_err(whoami, retval, bfr);
	    break;
	}
	if ((retval = ZIfNotice(&retnotice, (struct sockaddr_in *) 0,
				ZCompareUIDPred, 
				(char *)&notice->z_uid)) !=
	    ZERR_NONE) {
	    ZFreeNotice(&retnotice);
	    (void) sprintf(bfr, "while waiting for acknowledgement for %s", 
		    nrecips?notice->z_recipient:inst);
	    com_err(whoami, retval, bfr);
	    continue;
	}
	if (retnotice.z_kind == SERVNAK) {
	    printf("Received authorization failure while sending to %s\n", 
		   nrecips?notice->z_recipient:inst);
	    ZFreeNotice(&retnotice);
	    break;			/* if auth fails, punt */
	} 
	if (retnotice.z_kind != SERVACK || !retnotice.z_message_len) {
	    printf("Detected server failure while receiving acknowledgement for %s\n", 
		   nrecips?notice->z_recipient:inst);
	    ZFreeNotice(&retnotice);
	    continue;
	}
	if (!real || (!quiet && real))
	    if (!strcmp(retnotice.z_message, ZSRVACK_SENT)) {
		if (real) {
		    if (verbose)
			printf("Successful\n");
		    else
			printf("%s: Message sent\n", 
			       nrecips?notice->z_recipient:inst);
		}
		else
		    success = 1;
	    } 
	    else
		if (!strcmp(retnotice.z_message, 
			    ZSRVACK_NOTSENT)) {
		    if (verbose && real) {
			if (strcmp(class, DEFAULT_CLASS))
			    printf("Not logged in or not subscribing to class %s, instance %s\n", 
				   class, inst);
			else
			    printf("Not logged in or not subscribing to messages\n");
		    } 
		    else
			if (!nrecips)
			    printf("No one subscribing to class %s, instance %s\n", 
				   class, inst);
			else {
			    if (strcmp(class, DEFAULT_CLASS))
				printf("%s: Not logged in or not subscribing to class %s, instance %s\n", 
				       notice->z_recipient, class, inst);
			    else
				printf("%s: Not logged in or not subscribing to messages\n", 
				       notice->z_recipient);
			} 
		} 
		else
		    printf("Internal failure - illegal message field in server response\n");
	ZFreeNotice(&retnotice);
	if (!nrecips)
	    break;
    }
    if (!real && !success)
	exit(1);
} 

usage(s)
    char *s;
{
    printf("Usage: %s [-a] [-o] [-d] [-v] [-q] [-n] [-t] [-u]\n\
\t[-c class] [-i inst] [-f fsname] [user ...] [-m message]\n", s);
    printf("\t-f and -c are mutually exclusive\n\
\t-f and -i are mutually exclusive\n");
    exit(1);
} 

/*
  if the -f option is specified, this routine is called to canonicalize
  an instance of the form hostname[:pack].  It turns the hostname into the
  name returned by gethostbyname(hostname)
 */

char *fix_filsrv_inst(str)
char *str;
{
	static char fsinst[BUFSIZ];
	char *ptr;
	struct hostent *hp;

	ptr = index(str,':');
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
un_tabify(bufp, sizep)
char **bufp;
register int *sizep;
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
