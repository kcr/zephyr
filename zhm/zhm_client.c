/* This file is part of the Project Athena Zephyr Notification System.
 * It contains the hostmanager <--> client interaction routines.
 *
 *      Created by:     David C. Jedlinsky
 *
 *      $Source: /srv/kcr/athena/zephyr/zhm/zhm_client.c,v $
 *      $Author: jtkohl $
 *
 *      Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *      For copying and distribution information, see the file
 *      "mit-copyright.h". 
 */

#include "zhm.h"

#ifndef lint
#ifndef SABER
static char rcsid_hm_client_c[] = "$Header: /srv/kcr/athena/zephyr/zhm/zhm_client.c,v 1.6 1988-06-23 14:39:29 jtkohl Exp $";
#endif SABER
#endif lint

extern int no_server, timeout_type, nclt, deactivated;
extern struct sockaddr_in cli_sin, serv_sin, from;

transmission_tower(notice, packet, pak_len)
     ZNotice_t *notice;
     caddr_t packet;
     int pak_len;
{
      ZNotice_t gack;
      Code_t ret;
      struct sockaddr_in gsin;
      unsigned int tleft;

      nclt++;
      if (notice->z_kind == HMCTL) {
	    if (!strcmp(notice->z_opcode, CLIENT_FLUSH)) {
		  send_flush_notice(HM_FLUSH);
		  deactivated = 1;
	  }
	    else if (!strcmp(notice->z_opcode, CLIENT_NEW_SERVER))
		    new_server((char *)NULL);
	    else
		    syslog (LOG_INFO, "Bad control notice from client.");
	    return;
    } else
	if (notice->z_kind != UNSAFE) {
	      gack = *notice;
	      gack.z_kind = HMACK;
	      gack.z_message_len = 0;
	      gack.z_multinotice = "";
	      gsin = cli_sin;
	      gsin.sin_port = from.sin_port;
	      if (gack.z_port == 0)
		gack.z_port = from.sin_port;
	      DPR2 ("Client Port = %u\n", ntohs(gack.z_port));
	      notice->z_port = gack.z_port;
	      if ((ret = ZSetDestAddr(&gsin)) != ZERR_NONE) {
		    Zperr(ret);
		    com_err("hm", ret, "setting destination");
	      }
	      /* Bounce ACK to library */
	      if ((ret = send_outgoing(&gack)) != ZERR_NONE) {
		    Zperr(ret);
		    com_err("hm", ret, "sending raw notice");
	      }
	}
      if (!no_server) {
	    DPR2 ("Server Port = %u\n", ntohs(serv_sin.sin_port));
	    if ((ret = ZSetDestAddr(&serv_sin)) != ZERR_NONE) {
		  Zperr(ret);
		  com_err("hm", ret, "setting destination");
	    }
	    if ((ret = send_outgoing(notice)) != ZERR_NONE) {
		  Zperr(ret);
		  com_err("hm", ret, "while sending raw notice");
	    }
	    if (tleft = alarm(0))
	      (void)alarm(tleft);
	    else {
		  timeout_type = NOTICES;
		  (void)alarm(NOTICE_TIMEOUT);
	    }
      }
      (void)add_notice_to_queue(notice, packet, &gsin, pak_len);
}

Code_t
send_outgoing(notice)
ZNotice_t *notice;
{
	Code_t retval;
	char *packet;
	int length;

	if (!(packet = (char *) malloc((unsigned)sizeof(ZPacket_t))))
		return(ENOMEM);

	if ((retval = ZFormatSmallRawNotice(notice, packet, &length))
	    != ZERR_NONE) {
		free(packet);
		return(retval);
	}
	retval = ZSendPacket(packet, length, 0);
	free(packet);
	return(retval);
}
