/* This file is part of the Project Athena Zephyr Notification System.
 * It contains the hostmanager <--> server interaction routines.
 *
 *      Created by:     David C. Jedlinsky
 *
 *      $Source: /srv/kcr/athena/zephyr/zhm/zhm_server.c,v $
 *      $Author: opus $
 *
 *      Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h". 
 */

#include "hm.h"

#ifndef lint
#ifndef SABER
static char rcsid_hm_server_c[] = "$Header: /srv/kcr/athena/zephyr/zhm/zhm_server.c,v 1.2 1987-10-07 15:30:45 opus Exp $";
#endif SABER
#endif lint

int serv_loop = 0;
extern u_short cli_port;
extern struct sockaddr_in serv_sin, from;
extern int timeout_type, hmdebug, nservchang, booting, nserv, no_server;
extern int deactivated;
extern char **serv_list, **cur_serv_list;
extern char cur_serv[], prim_serv[];

/* Argument is whether we are actually booting, or just attaching
   after a server switch */
send_boot_notice(op)
     char *op;
{
      ZNotice_t notice;
      Code_t ret;

      /* Set up server notice */
      notice.z_kind = HMCTL;
      notice.z_port = cli_port;
      notice.z_class = ZEPHYR_CTL_CLASS;
      notice.z_class_inst = ZEPHYR_CTL_HM;
      notice.z_opcode = op;
      notice.z_sender = "HM";
      notice.z_recipient = "";
      notice.z_default_format = 0;
      notice.z_message_len = 0;
      
      /* Notify server that this host is here */
      if ((ret = ZSetDestAddr(&serv_sin)) != ZERR_NONE) {
	    Zperr(ret);
	    com_err("hm", ret, "setting destination");
      }
      if ((ret = ZSendNotice(&notice, ZNOAUTH)) != ZERR_NONE) {
	    Zperr(ret);
	    com_err("hm", ret, "sending startup notice");
      }
      timeout_type = BOOTING;
      (void)alarm(SERV_TIMEOUT);
}

/* Argument is whether we are detaching or really going down */
send_flush_notice(op)
     char *op;
{
      ZNotice_t notice;
      Code_t ret;

      /* Set up server notice */
      notice.z_kind = HMCTL;
      notice.z_port = cli_port;
      notice.z_class = ZEPHYR_CTL_CLASS;
      notice.z_class_inst = ZEPHYR_CTL_HM;
      notice.z_opcode = op;
      notice.z_sender = "HM";
      notice.z_recipient = "";
      notice.z_default_format = 0;
      notice.z_message_len = 0;

      /* Tell server to lose us */
      if ((ret = ZSetDestAddr(&serv_sin)) != ZERR_NONE) {
	    Zperr(ret);
	    com_err("hm", ret, "setting destination");
      }
      if ((ret = ZSendNotice(&notice, ZNOAUTH)) != ZERR_NONE) {
	    Zperr(ret);
	    com_err("hm", ret, "sending flush notice");
      }
}

find_next_server(sugg_serv)
     char *sugg_serv;
{
      struct hostent *hp;
      int done = 0;
      char **parse = serv_list;

      if (sugg_serv) {
	    do {
		  if (!strcmp(*parse, sugg_serv))
		    done = 1;
	    } while ((done == 0) && (*++parse != NULL));
      }
      if (done) {
	    if (hmdebug)
	      syslog(LOG_DEBUG, "Suggested server: %s\n", sugg_serv);
	    hp = gethostbyname(sugg_serv);
	    DPR2 ("Server = %s\n", sugg_serv);
	    (void)strcpy(cur_serv, sugg_serv);
      } else {		  
	    if ((++serv_loop > 3) && (strcmp(cur_serv, prim_serv))) {
		  serv_loop = 0;
		  hp = gethostbyname(prim_serv);
		  DPR2 ("Server = %s\n", prim_serv);
		  (void)strcpy(cur_serv, prim_serv);
	    } else
	      do {
		    if (*++cur_serv_list == NULL)
		      cur_serv_list = serv_list;
		    if (strcmp(*cur_serv_list, cur_serv)) {
			  hp = gethostbyname(*cur_serv_list);
			  DPR2 ("Server = %s\n", *cur_serv_list);
			  (void)strcpy(cur_serv, *cur_serv_list);
			  done = 1;
		    }
	      } while (done == 0);
      }
      bcopy(hp->h_addr, &serv_sin.sin_addr, hp->h_length);
      nservchang++;
}

server_manager(notice)
     ZNotice_t *notice;
{
      if ((bcmp(&serv_sin.sin_addr, &from.sin_addr, 4) != 0) ||
	  (serv_sin.sin_port != from.sin_port)) {
	    syslog (LOG_INFO, "Bad notice from port %u.", notice->z_port);
      } else {
	    /* This is our server, handle the notice */
	    booting = 0;
	    DPR ("A notice came in from the server.\n");
	    nserv++;
	    switch(notice->z_kind) {
		case HMCTL:
		  hm_control(notice);
		  break;
		case SERVNAK:
		case SERVACK:
		  send_back(notice);
		  break;
		default:
		  syslog (LOG_INFO, "Bad notice kind!?");
		  break;
	    }
      }
}

hm_control(notice)
     ZNotice_t *notice;
{
      Code_t ret;
      struct hostent *hp;
      char suggested_server[64];
      long addr;

      DPR("Control message!\n");
      if (!strcmp(notice->z_opcode, SERVER_SHUTDOWN)) {
	      if (notice->z_message_len) {
		      addr = inet_addr(notice->z_message);
		      if ((hp = gethostbyaddr(&addr,
					      4,
					      AF_INET)) != NULL) {
			      (void)strcpy(suggested_server, hp->h_name);
			      new_server(suggested_server);
		      } else
			      new_server(NULL);
	      }
	      else
		      new_server(NULL);
      } else if (!strcmp(notice->z_opcode, SERVER_PING)) {
	    if (no_server)
	      (void)alarm(0);
	    notice->z_kind = HMACK;
	    if ((ret = ZSetDestAddr(&serv_sin)) != ZERR_NONE) {
		  Zperr(ret);
		  com_err("hm", ret, "setting destination");
	    }
	    if ((ret = ZSendRawNotice(notice)) != ZERR_NONE) {
		  Zperr(ret);
		  com_err("hm", ret, "sending ACK");
	    }
	    if (no_server) {
		  no_server = 0;
		  retransmit_queue(&serv_sin);
	    }
      } else
	syslog (LOG_INFO, "Bad control message.");
}

send_back(notice)
     ZNotice_t *notice;
{
      ZNotice_Kind_t kind;
      struct sockaddr_in repl;
      Code_t ret;

      if (no_server)
	(void)alarm(0);
      if (!strcmp(notice->z_opcode, HM_BOOT) ||
	  !strcmp(notice->z_opcode, HM_ATTACH)) {
	    /* ignore message, just an ack from boot */
      } else {
	    if (remove_notice_from_queue(notice, &kind,
					 &repl) != ZERR_NONE) {
		  syslog (LOG_INFO, "Hey! This packet isn't in my queue!");
	    } else {
		  /* check if client wants an ACK, and send it */
		  if (kind == ACKED) {
			DPR2 ("Client ACK port: %u\n", ntohs(repl.sin_port));
			if ((ret = ZSetDestAddr(&repl)) != ZERR_NONE) {
			      Zperr(ret);
			      com_err("hm", ret, "setting destination");
			}
			if ((ret = ZSendRawNotice(notice)) != ZERR_NONE) {
			      Zperr(ret);
			      com_err("hm", ret, "sending ACK");
			}
		  }
	    }
      }
      if (no_server) {
	    no_server = 0;
	    retransmit_queue(&serv_sin);
      }
}

new_server(sugg_serv)
     char *sugg_serv;
{
      no_server = 1;
      syslog (LOG_INFO, "Server went down, finding new server.");
      send_flush_notice(HM_DETACH);
      find_next_server(sugg_serv);
      if (booting) {
	    send_boot_notice(HM_BOOT);
	    deactivated = 0;
      } else
	send_boot_notice(HM_ATTACH);
}
