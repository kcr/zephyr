/* This file is part of the Project Athena Zephyr Notification System.
 * It contains code for the "zaway" command.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/clients/zaway/zaway.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr.h>

#include <pwd.h>

#ifndef lint
static char rcsid_zaway_c[] = "$Header: /srv/kcr/athena/zephyr/clients/zaway/zaway.c,v 1.1 1987-08-06 22:34:37 rfrench Exp $";
#endif lint

#define MESSAGE_CLASS "MESSAGE"
#define DEFAULT_MSG "I'm sorry, but I am currently away from the terminal and am\nnot able to receive your message.\n"

main(argc,argv)
	int argc;
	char *argv[];
{
	FILE *fp;
	ZNotice_t notice;
	ZPacket_t packet;
	ZSubscription_t sub;
	u_short port;
	int retval;
	struct passwd *pw;
	char awayfile[BUFSIZ],inrep[BUFSIZ],*ptr,*msg[2],*envptr;
	char *find_message();
	
	if ((retval = ZInitialize()) != ZERR_NONE) {
		com_err(argv[0],retval,"while initializing");
		exit(1);
	}

	port = 0;
	if ((retval = ZOpenPort(&port)) != ZERR_NONE) {
		com_err(argv[0],retval,"while opening port");
		exit(1);
	}

	sub.class = MESSAGE_CLASS;
	sub.classinst = "*";
	sub.recipient = ZGetSender();

	if (argc > 1)
		strcpy(awayfile,argv[1]);
	else {
		envptr = (char *)getenv("HOME");
		if (envptr)
			sprintf(awayfile,"%s/.away",envptr);
		else {
			if (!(pw = getpwuid(getuid()))) {
				fprintf(stderr,"Who are you?\n");
				exit(1);
			}
			sprintf(awayfile,"%s/.away",pw->pw_dir);
		} 
	}

	fp = fopen(awayfile,"r");
	if (!fp && argc > 1) {
		fprintf(stderr,"File %s not found!\n",awayfile);
		exit(1);
	} 
	
	if ((retval = ZSubscribeTo(&sub,1,port)) != ZERR_NONE) {
		com_err(argv[0],retval,"while subscribing");
		exit(1);
	}

	for (;;) {
		if ((retval = ZReceiveNotice(packet,sizeof packet,
					     &notice,0)) != ZERR_NONE) {
			com_err(argv[0],retval,"while receiving notice");
			continue;
		}
		if (!strcmp(notice.z_sender,ZGetSender()))
			continue;
		if (!strcmp(notice.z_message,"Automated reply:\n"))
			continue;
		if (fp) {
			if (!(ptr = find_message(&notice,fp)))
				continue;
		}
		else {
			ptr = (char *)malloc(sizeof(DEFAULT_MSG)+1);
			if (!ptr) {
				com_err(argv[0],errno,"while getting default message");
				exit(1);
			}
			strcpy(ptr,DEFAULT_MSG);
		}
		notice.z_recipient = notice.z_sender;
		notice.z_sender = 0;
		notice.z_default_format = 0;
		     
		msg[0] = "Automated reply:\n";
		msg[1] = ptr;
		
		notice.z_message_len = strlen(notice.z_message)+1;
		if ((retval = ZSendList(&notice,msg,2,ZNOAUTH)) != ZERR_NONE) {
			com_err(argv[0],retval,"while sending notice");
			continue;
		}
		free(ptr);
	}
}

char *find_message(notice,fp)
	ZNotice_t *notice;
	FILE *fp;
{
	char bfr[BUFSIZ],*ptr,*ptr2,sender[BUFSIZ];
	int gotone,lastwasnt;
	char *realloc();
	
	rewind(fp);

	strcpy(sender,notice->z_sender);
	ptr2 = (char *)index(sender,'@');
	if (ptr2)
		*ptr2 = '\0';
	
	ptr = 0;
	gotone = 0;
	lastwasnt = 0;
	
	while (fgets(bfr,sizeof bfr,fp) > 0) {
		if (*bfr == '>') {
			if (lastwasnt)
				gotone = 0;
			bfr[strlen(bfr)-1] = '\0';
			ptr2 = (char *)index(bfr,'@');
			if (ptr2)
				*ptr2 = '\0';
			if (!strcmp(bfr+1,sender) ||
			    !strcmp(bfr+1,"*") ||
			    (!strcmp(bfr+1,"%") && !ptr))
				gotone = 1;
			lastwasnt = 0;
		} 
		else {
			if (gotone) {
				if (!ptr) {
					ptr = (char *)malloc(strlen(bfr)+1);
					*ptr = '\0';
				} 
				else
					ptr = (char *)realloc(ptr,strlen(bfr)+
							      strlen(ptr)+1);
				strcat(ptr,bfr);
			}
			lastwasnt = 1;
		}
	}

	return (ptr);
}
