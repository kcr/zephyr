/*
 * Copyright (c) 1990 Dennis Ferguson.  All rights reserved.
 *
 * Commercial use is permitted only if products which are derived from
 * or include this software are made available for purchase and/or use
 * in Canada.  Otherwise, redistribution and use in source and binary
 * forms are permitted.
 */

/*
 * des_cbc_cksum.c - compute an 8 byte checksum using DES in CBC mode
 */
#include "des.h"
#include "f_tables.h"

unsigned long
des_cbc_cksum(in, out, length, schedule, ivec)
	des_cblock *in;
	des_cblock *out;
	long length;
	des_key_schedule schedule;
	des_cblock ivec;
{
	register unsigned long left, right;
	register unsigned long temp;
	register u_int32 *kp;
	register unsigned char *ip;
	register long len;

	/*
	 * Initialize left and right with the contents of the initial
	 * vector.
	 */
	ip = (unsigned char *)ivec;
	GET_HALF_BLOCK(left, ip);
	GET_HALF_BLOCK(right, ip);

	/*
	 * Suitably initialized, now work the length down 8 bytes
	 * at a time.
	 */
	ip = (unsigned char *)in;
	len = length;
	while (len > 0) {
		/*
		 * Get more input, xor it in.  If the length is
		 * greater than or equal to 8 this is straight
		 * forward.  Otherwise we have to fart around.
		 */
		if (len >= 8) {
			left ^= ((*ip++) & 0xff) << 24;
			left ^= ((*ip++) & 0xff) << 16;
			left ^= ((*ip++) & 0xff) << 8;
			left ^= (*ip++) & 0xff;
			right ^= ((*ip++) & 0xff) << 24;
			right ^= ((*ip++) & 0xff) << 16;
			right ^= ((*ip++) & 0xff) << 8;
			right ^= (*ip++) & 0xff;
			len -= 8;
		} else {
			/*
			 * Oh, shoot.  We need to pad the
			 * end with zeroes.  Work backwards
			 * to do this.
			 */
			ip += (int) len;
			switch(len) {
			case 7:
				right ^= (*(--ip) & 0xff) << 8;
			case 6:
				right ^= (*(--ip) & 0xff) << 16;
			case 5:
				right ^= (*(--ip) & 0xff) << 24;
			case 4:
				left ^= *(--ip) & 0xff;
			case 3:
				left ^= (*(--ip) & 0xff) << 8;
			case 2:
				left ^= (*(--ip) & 0xff) << 16;
			case 1:
				left ^= (*(--ip) & 0xff) << 24;
				break;
			}
			len = 0;
		}

		/*
		 * Encrypt what we have
		 */
		kp = (u_int32 *)schedule;
		DES_DO_ENCRYPT(left, right, temp, kp);
	}

	/*
	 * Done.  Left and right have the checksum.  Put it into
	 * the output.
	 */
	ip = (unsigned char *)out;
	PUT_HALF_BLOCK(left, ip);
	PUT_HALF_BLOCK(right, ip);

	/*
	 * Return right.  I'll bet the MIT code returns this
	 * inconsistantly (with the low order byte of the checksum
	 * not always in the low order byte of the long).  We won't.
	 */
	return right;
}
