/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZParseNotice function.
 *
 *	Created by:	Robert French
 *
 *	$Source: /srv/kcr/locker/zephyr/lib/zephyr/ZParseNot.c,v $
 *	$Author: raeburn $
 *
 *	Copyright (c) 1987 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h". 
 */
/* $Header: /srv/kcr/locker/zephyr/lib/zephyr/ZParseNot.c,v 1.18 1990-11-15 13:07:35 raeburn Exp $ */

#ifndef lint
static char rcsid_ZParseNotice_c[] =
    "$Header: /srv/kcr/locker/zephyr/lib/zephyr/ZParseNot.c,v 1.18 1990-11-15 13:07:35 raeburn Exp $";
#endif lint

#include <zephyr/mit-copyright.h>

#include <zephyr/zephyr_internal.h>

/* Assume that strlen is efficient on this machine... */
#define next_field(ptr)	ptr += strlen (ptr) + 1

#if defined (__GNUC__) && defined (__vax__)
#undef next_field
static __inline__ char * Istrend (char *str) {
    /*
     * This should be faster on VAX models outside the 2 series.  Don't
     * use it if you are using MicroVAX 2 servers.  If you are using a
     * VS2 server, use something like
     *	#define next_field(ptr)		while(*ptr++)
     * instead of this code.
     *
     * This requires use of GCC to get the optimized code, but
     * everybody uses GCC, don't they? :-)
     */
    register char *str2 asm ("r1");
    /* Assumes that no field is longer than 64K.... */
    asm ("locc $0,$65535,(%1)" : "=r" (str2) : "r" (str) : "r0");
    return str2;
}
#define next_field(ptr) ptr = Istrend (ptr) + 1
#endif

#ifdef mips
#undef next_field
/*
 * The compiler doesn't optimize this macro as well as it does the
 * following function.
 */
#define next_fieldXXX(ptr) do{register unsigned c1,c2;c1= *ptr;	\
		   while((ptr++,c2= *ptr,c1)&&(ptr++,c1= *ptr,c2));}while(0)
static char *next_field_1 (s) char *s; {
    /*
     * Calling overhead is still present, but this routine is faster
     * than strlen, and doesn't bother with some of the other math
     * that we'd just have to undo later anyways.
     */
    register unsigned c1 = *s, c2;
    while (1) {
	s++; c2 = *s; if (c1 == 0) break;
	s++; c1 = *s; if (c2 == 0) break;
	s++; c2 = *s; if (c1 == 0) break;
	s++; c1 = *s; if (c2 == 0) break;
    }
    return s;
}
#define next_field(ptr)	ptr=next_field_1(ptr)
#endif

static int zzz = ZERR_BADPKT;
#undef ZERR_BADPKT
#define ZERR_BADPKT (abort(),zzz)

Code_t ZParseNotice(buffer, len, notice)
    char *buffer;
    int len;
    ZNotice_t *notice;
{
    char *ptr, *end;
    int maj, numfields, i;
    union {
	int i;
	ZUnique_Id_t uid;
	u_short us;
	ZChecksum_t sum;
    } temp;

    bzero((char *)notice, sizeof(ZNotice_t));
	
    ptr = buffer;
    end = buffer+len;

    notice->z_packet = buffer;
    
    notice->z_version = ptr;
    if (strncmp(ptr, ZVERSIONHDR, sizeof(ZVERSIONHDR) - 1))
	return (ZERR_VERS);
    ptr += sizeof(ZVERSIONHDR) - 1;
    maj = atoi(ptr);
    if (maj != ZVERSIONMAJOR)
	return (ZERR_VERS);
    next_field (ptr);

    if (ZReadAscii(ptr, end-ptr, (unsigned char *)&temp.i,
		   sizeof(temp.i)) == ZERR_BADFIELD)
	return (ZERR_BADPKT);
    numfields = ntohl((u_long) temp.i);
    next_field (ptr);

    /*XXX 3 */
    numfields -= 2; /* numfields, version, and checksum */
    if (numfields < 0)
	return (ZERR_BADPKT);

    if (numfields) {
	if (ZReadAscii(ptr, end-ptr, (unsigned char *)&temp.i,
		       sizeof(temp.i)) == ZERR_BADFIELD)
	    return (ZERR_BADPKT);
	notice->z_kind = (ZNotice_Kind_t)ntohl((u_long) temp.i);
	numfields--;
	next_field (ptr);
    }
    else
	return (ZERR_BADPKT);
	
    if (numfields) {
	if (ZReadAscii(ptr, end-ptr, (unsigned char *)&temp.uid,
		       sizeof(ZUnique_Id_t)) == ZERR_BADFIELD)
	    return (ZERR_BADPKT);
	notice->z_uid = temp.uid;
	notice->z_time.tv_sec = ntohl((u_long) notice->z_uid.tv.tv_sec);
	notice->z_time.tv_usec = ntohl((u_long) notice->z_uid.tv.tv_usec);
	numfields--;
	next_field (ptr);
    }
    else
	return (ZERR_BADPKT);
	
    if (numfields) {
	if (ZReadAscii(ptr, end-ptr, (unsigned char *)&temp.us,
		       sizeof(u_short)) ==
	    ZERR_BADFIELD)
	    return (ZERR_BADPKT);
	notice->z_port = temp.us;
	numfields--;
	next_field (ptr);
    }
    else
	return (ZERR_BADPKT);

    if (numfields) {
	if (ZReadAscii(ptr, end-ptr, (unsigned char *)&temp.i,
		       sizeof(int)) == ZERR_BADFIELD)
	    return (ZERR_BADPKT);
	notice->z_auth = temp.i;
	numfields--;
	next_field (ptr);
    }
    else
	return (ZERR_BADPKT);
	
    if (numfields) {
	if (ZReadAscii(ptr, end-ptr, (unsigned char *)&temp.i,
		       sizeof(int)) == ZERR_BADFIELD)
	    return (ZERR_BADPKT);
	notice->z_authent_len = ntohl((u_long) temp.i);
	numfields--;
	next_field (ptr);
    }
    else
	return (ZERR_BADPKT);

    if (numfields) {
	notice->z_ascii_authent = ptr;
	numfields--;
	next_field (ptr);
    }
    else
	return (ZERR_BADPKT);

    if (numfields) {
	notice->z_class = ptr;
	numfields--;
	next_field (ptr);
    }
    else
	notice->z_class = "";
	
    if (numfields) {
	notice->z_class_inst = ptr;
	numfields--;
	next_field (ptr);
    }
    else
	notice->z_class_inst = "";

    if (numfields) {
	notice->z_opcode = ptr;
	numfields--;
	next_field (ptr);
    }
    else
	notice->z_opcode = "";

    if (numfields) {
	notice->z_sender = ptr;
	numfields--;
	next_field (ptr);
    }
    else
	notice->z_sender = "";

    if (numfields) {
	notice->z_recipient = ptr;
	numfields--;
	next_field (ptr);
    }
    else
	notice->z_recipient = "";

    if (numfields) {
	notice->z_default_format = ptr;
	numfields--;
	next_field (ptr);
    }
    else
	notice->z_default_format = "";
	
/*XXX*/
    if (ZReadAscii(ptr, end-ptr, (unsigned char *)&temp.sum,
		   sizeof(ZChecksum_t))
	== ZERR_BADFIELD)
	return (ZERR_BADPKT);
    notice->z_checksum = ntohl((u_long) temp.sum);
    numfields--;
    next_field (ptr);

    if (numfields) {
	notice->z_multinotice = ptr;
	numfields--;
	next_field (ptr);
    }
    else
	notice->z_multinotice = "";

    if (numfields) {
	if (ZReadAscii(ptr, end-ptr, (unsigned char *)&temp.uid,
		       sizeof(ZUnique_Id_t)) == ZERR_BADFIELD)
	    return (ZERR_BADPKT);
	notice->z_multiuid = temp.uid;
	notice->z_time.tv_sec = ntohl((u_long) notice->z_multiuid.tv.tv_sec);
	notice->z_time.tv_usec = ntohl((u_long) notice->z_multiuid.tv.tv_usec);
	numfields--;
	next_field (ptr);
    }
    else
	notice->z_uid = notice->z_multiuid;

    for (i=0;i<Z_MAXOTHERFIELDS && numfields;i++,numfields--) {
	notice->z_other_fields[i] = ptr;
	numfields--;
	next_field (ptr);
    }
    notice->z_num_other_fields = i;
    
    for (i=0;i<numfields;i++)
	next_field (ptr);
	
#ifdef notdef
    if (ZReadAscii(ptr, end-ptr, (unsigned char *)temp, 
		   sizeof(ZChecksum_t))
	== ZERR_BADFIELD)
	return (ZERR_BADPKT);
    notice->z_checksum = ntohl(*temp);
    numfields--;
    next_field (ptr);
#endif
    
    notice->z_message = (caddr_t) ptr;
    notice->z_message_len = len-(ptr-buffer);

    return (ZERR_NONE);
}
