/* This file is part of the Project Athena Zephyr Notification System.
 * It contains the zstat program.
 *
 *      Created by:     David C. Jedlinsky
 *
 *      $Source: /srv/kcr/locker/zephyr/clients/zstat/zstat.c,v $
 *      $Author: jtkohl $
 *
 *      Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h". 
 */

#include <zephyr/zephyr.h>
#include "../server/zserver.h"
#include <sys/param.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>

#ifndef lint
#ifndef SABER
static char rcsid_zstat_c[] = "$Header: /srv/kcr/locker/zephyr/clients/zstat/zstat.c,v 1.2 1987-07-29 15:01:32 jtkohl Exp $";
#endif SABER
#endif lint
		     
char *head[20] = { "Current server =",
		     "Items in queue:",
		     "Client packets received:",
		     "Server packets received:",
		     "Server changes:",
		     "Version:",
		     "Looking for a new server:",
		     "Time running:",
		     "Size:",
		     "Machine type:"
};
char *srv_head[20] = { 
	"Current server version =",
	"Packets handled:",
	"Uptime:",
	"Server states:",
};

int serveronly = 0,hmonly = 0;
u_short hm_port,srv_port;

main(argc, argv)
	int argc;
	char *argv[];
{
	ZNotice_t notice;
	Code_t ret;
	caddr_t packet, mp;
	char hostname[MAXHOSTNAMELEN];
	char *host = NULL;
	char *srv_name;
	char optchar;
	struct servent *sp;
	int ml, i, nitems;
	extern char *optarg;
	extern int optind;

	if ((ret = ZInitialize()) != ZERR_NONE) {
		com_err("zstat", ret, "initializing");
		exit(-1);
	}

	if ((ret = ZOpenPort(0)) != ZERR_NONE) {
		com_err("zstat", ret, "opening port");
		exit(-1);
	}

	while ((optchar = getopt(argc, argv, "sh")) != EOF) {
		switch(optchar) {
		case 's':
			serveronly++;
			break;
		case 'h':
			hmonly++;
			break;
		case '?':
		default:
			usage(argv[0]);
			exit(1);
		}
	}

	if (serveronly && hmonly) {
		fprintf(stderr,"Only one of -s and -h may be specified\n");
		exit(1);
	}

	if (!(sp = getservbyname("zephyr-hm","udp"))) {
		fprintf(stderr,"zephyr-hm/udp: unknown service\n");
		exit(-1);
	}

	hm_port = sp->s_port;

	if (!(sp = getservbyname("zephyr-clt","udp"))) {
		fprintf(stderr,"zephyr-clt/udp: unknown service\n");
		exit(-1);
	}

	srv_port = sp->s_port;

	if (optind == argc) {
		if (gethostname(hostname, MAXHOSTNAMELEN) < 0) {
			com_err("zstat",errno,"while finding hostname");
			exit(-1);
		}
		do_stat(hostname);
		exit(0);
	}

	for (;optind<argc;optind++)
		do_stat(argv[optind]);

	exit(0);
}

do_stat(host)
	char *host;
{
	char srv_host[MAXHOSTNAMELEN];
	
	if (serveronly) {
		srv_stat(host);
		return;
	}

	hm_stat(host,srv_host);

	if (!hmonly)
		srv_stat(srv_host);
}

hm_stat(host,server)
	char *host,*server;
{
	char *line[20],*mp;
	int sock,i,nf,ret;
	struct hostent *hp;
	struct sockaddr_in sin;
	long runtime;
	struct tm *tim;
	ZNotice_t notice;
	ZPacket_t packet;
	
	bzero(&sin,sizeof(struct sockaddr_in));

	sin.sin_port = hm_port;

	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket:");
		exit(-1);
	}
	
	sin.sin_family = AF_INET;

	if ((hp = gethostbyname(host)) == NULL) {
		fprintf(stderr,"Unknown host: %s\n",host);
		exit(-1);
	}
	bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);

	printf("Hostmanager stats: %s\n",hp->h_name);
	
	notice.z_kind = STAT;
	notice.z_port = 0;
	notice.z_class = HM_STAT_CLASS;
	notice.z_class_inst = HM_STAT_CLIENT;
	notice.z_opcode = HM_GIMMESTATS;
	notice.z_sender = "";
	notice.z_recipient = "";
	notice.z_message_len = 0;
	
	if ((ret = ZSetDestAddr(&sin)) != ZERR_NONE) {
		com_err("zstat", ret, "setting destination");
		exit(-1);
	}
	if ((ret = ZSendNotice(&notice, ZNOAUTH)) != ZERR_NONE) {
		com_err("zstat", ret, "sending notice");
		exit(-1);
	}

	if ((ret = ZReceiveNotice(packet, sizeof packet, &notice,
				  NULL, NULL)) != ZERR_NONE) {
		com_err("zstat", ret, "receiving notice");
		exit(1);
	}

	mp = notice.z_message;
	for (nf=0;mp<notice.z_message+notice.z_message_len;nf++) {
		line[nf] = mp;
		mp += strlen(mp)+1;
	}

	strcpy(server,line[0]);

	for (i=0;i<nf;i++) {
		if (!strncmp("Time",head[i],4)) {
			runtime = atol(line[i]);
			tim = gmtime(&runtime);
			printf("%s %d days, %02d:%02d:%02d", head[i],
				tim->tm_yday,
				tim->tm_hour,
				tim->tm_min,
				tim->tm_sec);
		}
		else
			printf("%s %s\n",head[i],line[i]);
	}

	printf("\n");
	
	close(sock);
}

srv_stat(host)
	char *host;
{
	char *line[20],*mp;
	int sock,i,nf,ret;
	struct hostent *hp;
	struct sockaddr_in sin;
	ZNotice_t notice;
	ZPacket_t packet;
	long runtime;
	struct tm *tim;
	
	bzero(&sin,sizeof(struct sockaddr_in));

	sin.sin_port = srv_port;

	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket:");
		exit(-1);
	}
	
	sin.sin_family = AF_INET;

	if ((hp = gethostbyname(host)) == NULL) {
		fprintf(stderr,"Unknown host: %s\n",host);
		exit(-1);
	}
	bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);

	printf("Server stats: %s\n",hp->h_name);
	
	notice.z_kind = UNACKED;
	notice.z_port = 0;
	notice.z_class = ZEPHYR_ADMIN_CLASS;
	notice.z_class_inst = "";
	notice.z_opcode = ADMIN_STATUS;
	notice.z_sender = "";
	notice.z_recipient = "";
	notice.z_message_len = 0;
	
	if ((ret = ZSetDestAddr(&sin)) != ZERR_NONE) {
		com_err("zstat", ret, "setting destination");
		exit(-1);
	}
	if ((ret = ZSendNotice(&notice, ZNOAUTH)) != ZERR_NONE) {
		com_err("zstat", ret, "sending notice");
		exit(-1);
	}

	if ((ret = ZReceiveNotice(packet, sizeof packet, &notice,
				  NULL, NULL)) != ZERR_NONE) {
		com_err("zstat", ret, "receiving notice");
		exit(1);
	}

	mp = notice.z_message;
	for (nf=0;mp<notice.z_message+notice.z_message_len;nf++) {
		line[nf] = mp;
		mp += strlen(mp)+1;
	}

	for (i=0; i<nf; i++) {
		if (i < 2)
			printf("%s %s\n",srv_head[i],line[i]);
		else if (i == 2) { /* uptime field */
			runtime = atol(line[i]);
			tim = gmtime(&runtime);
			printf("%s %d days, %02d:%02d:%02d\n",
			       srv_head[i],
			       tim->tm_yday,
			       tim->tm_hour,
			       tim->tm_min,
			       tim->tm_sec);
		} else if (i == 3) {
			printf("%s\n",srv_head[i]);
			printf("%s\n",line[i]);
		} else printf("%s\n",line[i]);
	}
	close(sock);
}

usage(s)
	char *s;
{
	fprintf(stderr,"usage: %s [-s] [-h] [host ...]\n",s);
	exit(1);
}
