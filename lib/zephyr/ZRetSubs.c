/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZRetrieveSubscriptions function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZRetSubs.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZRetSubs.c,v 1.6 1987-11-08 00:22:50 rfrench Exp $ */

#ifndef lint
static char rcsid_ZRetrieveSubscriptions_c[] = "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZRetSubs.c,v 1.6 1987-11-08 00:22:50 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

#define MAXSUBPACKETS 200 /* If a person has more than 1000
			   *  subscriptions, he loses! */

Code_t ZRetrieveSubscriptions(port,nsubs)
	u_short port;
	int *nsubs;
{
	int subscription_pred();
	
	int i,retval,nrecv,totalrecv,npacket,thispacket,returncode,gimmeack;
	ZNotice_t notice,retnotice;
	ZPacket_t buffer;
	char *ptr,*end,*ptr2;
	char subs_recvd[MAXSUBPACKETS],asciiport[50];
	
	retval = ZFlushSubscriptions();

	if (retval != ZERR_NONE && retval != ZERR_NOSUBSCRIPTIONS)
		return (retval);
	
	notice.z_kind = ACKED;
	notice.z_port = 0;
	notice.z_class = ZEPHYR_CTL_CLASS;
	notice.z_class_inst = ZEPHYR_CTL_CLIENT;
	notice.z_opcode = CLIENT_GIMMESUBS;
	notice.z_sender = 0;
	notice.z_recipient = "";
	notice.z_default_format = 0;
	notice.z_message = asciiport;
	if ((retval = ZMakeAscii(asciiport,sizeof(asciiport),
				 (unsigned char *)&port,
				 sizeof(u_short))) != ZERR_NONE)
		return (retval);
	notice.z_message_len = strlen(asciiport)+1;

	if ((retval = ZSendNotice(&notice,ZAUTH)) != ZERR_NONE)
		return (retval);

	for (i=0;i<MAXSUBPACKETS;i++)
		subs_recvd[i] = 'n';
	
	nrecv = 0;
	totalrecv = -1;
	gimmeack = 0;
	__subscriptions_num = 0;
	__subscriptions_list = (ZSubscription_t *) 0;

	returncode = ZERR_NONE;

	while (nrecv < totalrecv || totalrecv == -1 || !gimmeack) {
		if ((retval = ZIfNotice(buffer,sizeof buffer,&retnotice,NULL,
					ZCompareUIDPred,
					(char *)&notice.z_uid)) !=
		    ZERR_NONE)
			return (retval);

		if (retnotice.z_kind == SERVNAK)
			return (ZERR_SERVNAK);
	
		if (retnotice.z_kind != SERVACK)
			return (ZERR_INTERNAL);

		if (!strcmp(retnotice.z_opcode,CLIENT_GIMMESUBS)) {
			gimmeack = 1;
			continue;
		} 
		ptr = index(retnotice.z_opcode,'/');
		if (!ptr)
			return (ZERR_INTERNAL);
		*ptr = '\0';
		totalrecv = atoi(ptr+1);
		if (totalrecv > MAXSUBPACKETS) {
			returncode = ZERR_TOOMANYSUBS;
			totalrecv = MAXSUBPACKETS;
		} 
		thispacket = atoi(retnotice.z_opcode);
		if (thispacket > MAXSUBPACKETS)
			continue;
		
		if (subs_recvd[thispacket] == 'y')
			continue;
		subs_recvd[thispacket] = 'y';
		nrecv++;

		end = retnotice.z_message+retnotice.z_message_len;

		npacket = 0;
		for (ptr=retnotice.z_message;ptr<end;ptr++)
			if (!*ptr)
				npacket++;

		npacket /= 3;

		__subscriptions_list = (ZSubscription_t *)
			realloc(__subscriptions_list,
				(unsigned)(__subscriptions_num+npacket)*
			       sizeof(ZSubscription_t));
		if (!__subscriptions_list)
			return (ENOMEM);
	
		for (ptr=retnotice.z_message,i=__subscriptions_num;
		     i<__subscriptions_num+npacket;i++) {
			__subscriptions_list[i].class = (char *)
				malloc((unsigned)strlen(ptr)+1);
			if (!__subscriptions_list[i].class)
				return (ENOMEM);
			(void) strcpy(__subscriptions_list[i].class,ptr);
			ptr += strlen(ptr)+1;
			__subscriptions_list[i].classinst = (char *)
				malloc((unsigned)strlen(ptr)+1);
			if (!__subscriptions_list[i].classinst)
				return (ENOMEM);
			(void) strcpy(__subscriptions_list[i].classinst,ptr);
			ptr += strlen(ptr)+1;
			ptr2 = ptr;
			if (!*ptr2)
				ptr2 = "*";
			__subscriptions_list[i].recipient = (char *)
				malloc((unsigned)strlen(ptr2)+1);
			if (!__subscriptions_list[i].recipient)
				return (ENOMEM);
			(void) strcpy(__subscriptions_list[i].recipient,ptr2);
			ptr += strlen(ptr)+1;
		}

		__subscriptions_num += npacket;
	} 

	__subscriptions_next = 0;
	*nsubs = __subscriptions_num;

	return (returncode);
}
