/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the internal Zephyr routines.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/athena/zephyr/lib/Zinternal.c,v $
 *	$Author: rfrench $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/athena/zephyr/lib/Zinternal.c,v 1.3 1988-05-13 13:07:31 rfrench Exp $ */

#ifndef lint
static char rcsid_Zinternal_c[] = "$Header: /srv/kcr/athena/zephyr/lib/Zinternal.c,v 1.3 1988-05-13 13:07:31 rfrench Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <utmp.h>

int __Zephyr_fd = -1;
int __Zephyr_open = 0;
int __Zephyr_port = -1;
int __My_length;
char *__My_addr;
int __Q_Length = 0;
struct _Z_InputQ *__Q_Head = 0, *__Q_Tail = 0;
struct sockaddr_in __HM_addr;
int __HM_set = 0;
C_Block __Zephyr_session;
int __Zephyr_server = 0;
char __Zephyr_realm[REALM_SZ];
ZLocations_t *__locate_list = 0;
int __locate_num = 0;
int __locate_next = 0;
ZSubscription_t *__subscriptions_list = 0;
int __subscriptions_num = 0;
int __subscriptions_next = 0;

Code_t Z_GetMyAddr()
{
	struct hostent *myhost;
	char hostname[BUFSIZ];
	struct hostent *gethostbyname();
	
	if (__My_length > 0)
		return (ZERR_NONE);

	if (gethostname(hostname,BUFSIZ) < 0)
		return (errno);

	if (!(myhost = gethostbyname(hostname)))
		return (errno);

	if (!(__My_addr = (char *)malloc((unsigned)myhost->h_length)))
		return (ENOMEM);

	__My_length = myhost->h_length;

	bcopy(myhost->h_addr,__My_addr,myhost->h_length);

	return (ZERR_NONE);
} 
	
Z_NoticeWaiting()
{
	int bytes;

	if (ioctl(ZGetFD(),FIONREAD,(char *)&bytes) < 0)
		return (0);

	return (bytes > 0);
} 

Z_ReadEnqueue()
{
	int retval;
	
	while (Z_NoticeWaiting())
		if ((retval = Z_ReadWait()) != ZERR_NONE)
			return (retval);

	return (ZERR_NONE);
}

Z_ReadWait()
{
	struct _Z_InputQ *newqueue;
	ZNotice_t notice;
	struct sockaddr_in olddest;
	int from_len,retval;
	
	if (ZGetFD() < 0)
		return (ZERR_NOPORT);
	
	if (__Q_Length > Z_MAXQLEN)
		return (ZERR_QLEN);
	
	newqueue = (struct _Z_InputQ *)malloc(sizeof(struct _Z_InputQ));
	if (!newqueue)
		return (ENOMEM);

	from_len = sizeof(struct sockaddr_in);
	
	newqueue->packet_len = recvfrom(ZGetFD(),newqueue->packet,
					sizeof newqueue->packet, 0,
					&newqueue->from,
					&from_len);

	if (newqueue->packet_len < 0) {
		free((char *)newqueue);
		return (errno);
	}

	if (!newqueue->packet_len) {
		free((char *)newqueue);
		return (ZERR_EOF);
	}
	
	if (!__Zephyr_server &&
	    ZParseNotice(newqueue->packet, newqueue->packet_len,
			 &notice) == ZERR_NONE) {
		if (notice.z_kind != HMACK && notice.z_kind != SERVACK &&
		    notice.z_kind != SERVNAK) {
			notice.z_kind = CLIENTACK;
			notice.z_message_len = 0;
			olddest = __HM_addr;
			__HM_addr = newqueue->from;
			if ((retval = ZSendRawNotice(&notice)) != ZERR_NONE)
				return (retval);
			__HM_addr = olddest;
		} 
	}
	
        newqueue->next = NULL;
	if (__Q_Length) {
		newqueue->prev = __Q_Tail;
		__Q_Tail->next = newqueue;
		__Q_Tail = newqueue;
	}
	else {
		newqueue->prev = NULL;
		__Q_Head = __Q_Tail = newqueue;
	}
	
	__Q_Length++;
	
	return (ZERR_NONE);
}

Z_FormatHeader(notice,buffer,buffer_len,len,cert_routine)
	ZNotice_t	*notice;
	char		*buffer;
	int		buffer_len;
	int		*len;
	int		(*cert_routine)();
{
	int retval;
	
	if (!notice->z_class || !notice->z_class_inst || !notice->z_opcode ||
	    !notice->z_recipient)
		return (ZERR_ILLVAL);

	if (!notice->z_sender)
		notice->z_sender = ZGetSender();

	(void) gettimeofday(&notice->z_uid.tv,(struct timezone *)0);
	notice->z_uid.tv.tv_sec = htonl(notice->z_uid.tv.tv_sec);
	notice->z_uid.tv.tv_usec = htonl(notice->z_uid.tv.tv_usec);
	
	if ((retval = Z_GetMyAddr()) != ZERR_NONE)
		return (retval);

	bcopy(__My_addr,(char *)&notice->z_uid.zuid_addr,__My_length);

	if (!cert_routine) {
		notice->z_auth = 0;
		notice->z_authent_len = 0;
		notice->z_ascii_authent = (char *)"";
		return (Z_FormatRawHeader(notice,buffer,buffer_len,len));
	}
	
	return ((cert_routine)(notice,buffer,buffer_len,len));
} 
	
Z_FormatRawHeader(notice,buffer,buffer_len,len)
	ZNotice_t	*notice;
	char		*buffer;
	int		buffer_len;
	int		*len;
{
	unsigned int temp;
	char newrecip[BUFSIZ],version[BUFSIZ];
	char *ptr,*end;

	if (!notice->z_class)
		notice->z_class = "";

	if (!notice->z_class_inst)
		notice->z_class_inst = "";

	if (!notice->z_opcode)
		notice->z_opcode = "";
	
	if (!notice->z_recipient)
		notice->z_recipient = "";

	if (!notice->z_default_format)
		notice->z_default_format = "";

	ptr = buffer;
	end = buffer+buffer_len;

	sprintf(version,"%s%d.%d",ZVERSIONHDR,ZVERSIONMAJOR,ZVERSIONMINOR);
	if (buffer_len < strlen(version)+1)
		return (ZERR_PKTLEN);

	strcpy(ptr,version);
	ptr += strlen(ptr)+1;

	temp = htonl(ZNUMFIELDS);
	if (ZMakeAscii(ptr,end-ptr,(unsigned char *)&temp,
		       sizeof(int)) == ZERR_FIELDLEN)
		return (ZERR_PKTLEN);
	ptr += strlen(ptr)+1;
	
	temp = htonl((int)notice->z_kind);
	if (ZMakeAscii(ptr,end-ptr,(unsigned char *)&temp,
		       sizeof(int)) == ZERR_FIELDLEN)
		return (ZERR_PKTLEN);
	ptr += strlen(ptr)+1;
	
	if (ZMakeAscii(ptr,end-ptr,(unsigned char *)&notice->z_uid,
		       sizeof(ZUnique_Id_t)) == ZERR_FIELDLEN)
		return (ZERR_PKTLEN);
	ptr += strlen(ptr)+1;
	
	if (ZMakeAscii(ptr,end-ptr,(unsigned char *)&notice->z_port,
		       sizeof(u_short)) == ZERR_FIELDLEN)
		return (ZERR_PKTLEN);
	ptr += strlen(ptr)+1;

	if (ZMakeAscii(ptr,end-ptr,(unsigned char *)&notice->z_auth,
		       sizeof(int)) == ZERR_FIELDLEN)
		return (ZERR_PKTLEN);
	ptr += strlen(ptr)+1;

	temp = htonl(notice->z_authent_len);
	if (ZMakeAscii(ptr,end-ptr,(unsigned char *)&temp,
		       sizeof(int)) == ZERR_FIELDLEN)
		return (ZERR_PKTLEN);
	ptr += strlen(ptr)+1;
	
	if (Z_AddField(&ptr,notice->z_ascii_authent,end))
		return (ZERR_PKTLEN);
	if (Z_AddField(&ptr,notice->z_class,end))
		return (ZERR_PKTLEN);
	if (Z_AddField(&ptr,notice->z_class_inst,end))
		return (ZERR_PKTLEN);
	if (Z_AddField(&ptr,notice->z_opcode,end))
		return (ZERR_PKTLEN);
	if (Z_AddField(&ptr,notice->z_sender,end))
		return (ZERR_PKTLEN);
	if (index(notice->z_recipient,'@') || !*notice->z_recipient) {
		if (Z_AddField(&ptr,notice->z_recipient,end))
			return (ZERR_PKTLEN);
	}
	else {
		(void) sprintf(newrecip,"%s@%s",notice->z_recipient,
			__Zephyr_realm);
		if (Z_AddField(&ptr,newrecip,end))
			return (ZERR_PKTLEN);
	}		
	if (Z_AddField(&ptr,notice->z_default_format,end))
		return (ZERR_PKTLEN);

	temp = htonl(notice->z_checksum);
	if (ZMakeAscii(ptr,end-ptr,(unsigned char *)&temp,
		       sizeof(ZChecksum_t)) == ZERR_FIELDLEN)
		return (ZERR_PKTLEN);
	ptr += strlen(ptr)+1;

	if (Z_AddField(&ptr,notice->z_multinotice,end))
		return (ZERR_PKTLEN);
	
	*len = ptr-buffer;
	
	return (ZERR_NONE);
}

Z_AddField(ptr,field,end)
	char **ptr,*field,*end;
{
	register int len;

	len = strlen(field)+1;

	if (*ptr+len > end)
		return (1);
	(void) strcpy(*ptr,field);
	*ptr += len;

	return (0);
}

Z_RemQueue(qptr)
	struct _Z_InputQ *qptr;
{
	__Q_Length--;

	if (!__Q_Length) {
		free ((char *)qptr);
		return (ZERR_NONE);
	} 
	
	if (qptr == __Q_Head) {
		__Q_Head = __Q_Head->next;
		__Q_Head->prev = NULL;
		free ((char *)qptr);
		return (ZERR_NONE);
	} 
	if (qptr == __Q_Tail) {
		__Q_Tail = __Q_Tail->prev;
		__Q_Tail->next = NULL;
		free ((char *)qptr);
		return (ZERR_NONE);
	}
	qptr->prev->next = qptr->next;
	qptr->next->prev = qptr->prev;
	free ((char *)qptr);
	return (ZERR_NONE);
}

Code_t Z_InternalParseNotice(buffer,len,notice)
	ZPacket_t	buffer;
	int		len;
	ZNotice_t	*notice;
{
	char *ptr,*end;
	int maj,numfields,i;
	unsigned int temp[3];

	bzero(notice,sizeof(ZNotice_t));
	
	ptr = buffer;
	end = buffer+len;
	
	notice->z_version = ptr;
	if (strncmp(ptr,ZVERSIONHDR,strlen(ZVERSIONHDR)))
		return (ZERR_VERS);
	ptr += strlen(ZVERSIONHDR);
	maj = atoi(ptr);
	if (maj != ZVERSIONMAJOR)
		return (ZERR_VERS);
	ptr += strlen(ptr)+1;

	if (ZReadAscii(ptr,end-ptr,(unsigned char *)temp,sizeof(int)) == ZERR_BADFIELD)
		return (ZERR_BADPKT);
	numfields = ntohl(*temp);
	ptr += strlen(ptr)+1;

	numfields -= 2;
	if (numfields < 0)
		numfields = 0;

	if (numfields) {
		if (ZReadAscii(ptr,end-ptr,(unsigned char *)temp,
			       sizeof(int)) == ZERR_BADFIELD)
			return (ZERR_BADPKT);
		notice->z_kind = (ZNotice_Kind_t)ntohl((ZNotice_Kind_t)*temp);
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		return (ZERR_BADPKT);
	
	if (numfields) {
		if (ZReadAscii(ptr,end-ptr,(unsigned char *)temp,
			       sizeof(ZUnique_Id_t)) ==
		    ZERR_BADFIELD)
			return (ZERR_BADPKT);
		bcopy((char *)temp,(char *)&notice->z_uid,sizeof(ZUnique_Id_t));
		notice->z_time.tv_sec = ntohl(notice->z_uid.tv.tv_sec);
		notice->z_time.tv_usec = ntohl(notice->z_uid.tv.tv_usec);
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		return (ZERR_BADPKT);
	
	if (numfields) {
		if (ZReadAscii(ptr,end-ptr,(unsigned char *)temp,
			       sizeof(u_short)) ==
		    ZERR_BADFIELD)
			return (ZERR_BADPKT);
		notice->z_port = *((u_short *)temp);
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		return (ZERR_BADPKT);

	if (numfields) {
		if (ZReadAscii(ptr,end-ptr,(unsigned char *)temp,
			       sizeof(int)) == ZERR_BADFIELD)
			return (ZERR_BADPKT);
		notice->z_auth = *temp;
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		return (ZERR_BADPKT);
	
	if (numfields) {
		if (ZReadAscii(ptr,end-ptr,(unsigned char *)temp,
			       sizeof(int)) == ZERR_BADFIELD)
			return (ZERR_BADPKT);
		notice->z_authent_len = ntohl(*temp);
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		return (ZERR_BADPKT);

	if (numfields) {
		notice->z_ascii_authent = ptr;
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		return (ZERR_BADPKT);

	if (numfields) {
		notice->z_class = ptr;
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		notice->z_class = "";
	
	if (numfields) {
		notice->z_class_inst = ptr;
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		notice->z_class_inst = "";

	if (numfields) {
		notice->z_opcode = ptr;
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		notice->z_opcode = "";

	if (numfields) {
		notice->z_sender = ptr;
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		notice->z_sender = "";

	if (numfields) {
		notice->z_recipient = ptr;
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		notice->z_recipient = "";

	if (numfields) {
		notice->z_default_format = ptr;
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		notice->z_default_format = "";
	
	if (numfields) {
		if (ZReadAscii(ptr,end-ptr,(unsigned char *)temp,
			       sizeof(ZChecksum_t))
		    == ZERR_BADFIELD)
			return (ZERR_BADPKT);
		notice->z_checksum = ntohl(*temp);
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		notice->z_checksum = 0;

	if (numfields) {
		notice->z_multinotice = ptr;
		numfields--;
		ptr += strlen(ptr)+1;
	}
	else
		notice->z_multinotice = "";

	for (i=0;i<numfields;i++)
		ptr += strlen(ptr)+1;
	
	notice->z_message = (caddr_t) ptr;
	notice->z_message_len = len-(ptr-buffer);

	return (ZERR_NONE);
}

