/*
 * Copyright (c) 1990 Dennis Ferguson.  All rights reserved.
 *
 * Commercial use is permitted only if products which are derived from
 * or include this software are made available for purchase and/or use
 * in Canada.  Otherwise, redistribution and use in source and binary
 * forms are permitted.
 */

/*
 * des_pcbc_encrypt.c - encrypt a string of characters in error propagation mode
 */
#include "des.h"
#include "f_tables.h"

/*
 * des_pcbc_encrypt - {en,de}crypt a stream in PCBC mode
 */
int
des_pcbc_encrypt(in, out, length, schedule, ivec, encrypt)
	des_cblock *in;
	des_cblock *out;
	long length;
	des_key_schedule schedule;
	des_cblock ivec;
	int encrypt;
{
	register unsigned long left, right;
	register unsigned long temp;
	register u_int32 *kp;
	register unsigned char *ip, *op;

	/*
	 * Copy the key pointer, just once
	 */
	kp = (u_int32 *)schedule;

	/*
	 * Deal with encryption and decryption separately.
	 */
	if (encrypt) {
		register unsigned long plainl;
		register unsigned long plainr;

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
		op = (unsigned char *)out;
		while (length > 0) {
			/*
			 * Get block of input.  If the length is
			 * greater than 8 this is straight
			 * forward.  Otherwise we have to fart around.
			 */
			if (length > 8) {
				GET_HALF_BLOCK(plainl, ip);
				GET_HALF_BLOCK(plainr, ip);
				left ^= plainl;
				right ^= plainr;
				length -= 8;
			} else {
				/*
				 * Oh, shoot.  We need to pad the
				 * end with zeroes.  Work backwards
				 * to do this.  We know this is the
				 * last block, though, so we don't have
				 * to save the plain text.
				 */
				ip += (int) length;
				switch(length) {
				case 8:
					right ^= *(--ip) & 0xff;
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
				length = 0;
			}

			/*
			 * Encrypt what we have
			 */
			DES_DO_ENCRYPT(left, right, temp, kp);

			/*
			 * Copy the results out
			 */
			PUT_HALF_BLOCK(left, op);
			PUT_HALF_BLOCK(right, op);

			/*
			 * Xor with the old plain text
			 */
			left ^= plainl;
			right ^= plainr;
		}
	} else {
		/*
		 * Decrypting is harder than encrypting because of
		 * the necessity of remembering a lot more things.
		 * Should think about this a little more...
		 */
		unsigned long ocipherl, ocipherr;
		unsigned long cipherl, cipherr;

		if (length <= 0)
			return 0;

		/*
		 * Prime the old cipher with ivec.
		 */
		ip = (unsigned char *)ivec;
		GET_HALF_BLOCK(ocipherl, ip);
		GET_HALF_BLOCK(ocipherr, ip);

		/*
		 * Now do this in earnest until we run out of length.
		 */
		ip = (unsigned char *)in;
		op = (unsigned char *)out;
		for (;;) {		/* check done inside loop */
			/*
			 * Read a block from the input into left and
			 * right.  Save this cipher block for later.
			 */
			GET_HALF_BLOCK(left, ip);
			GET_HALF_BLOCK(right, ip);
			cipherl = left;
			cipherr = right;

			/*
			 * Decrypt this.
			 */
			DES_DO_DECRYPT(left, right, temp, kp);

			/*
			 * Xor with the old cipher to get plain
			 * text.  Output 8 or less bytes of this.
			 */
			left ^= ocipherl;
			right ^= ocipherr;
			if (length > 8) {
				length -= 8;
				PUT_HALF_BLOCK(left, op);
				PUT_HALF_BLOCK(right, op);
				/*
				 * Save current cipher block here
				 */
				ocipherl = cipherl ^ left;
				ocipherr = cipherr ^ right;
			} else {
				/*
				 * Trouble here.  Start at end of output,
				 * work backwards.
				 */
				op += (int) length;
				switch(length) {
				case 8:
					*(--op) = right & 0xff;
				case 7:
					*(--op) = (right >> 8) & 0xff;
				case 6:
					*(--op) = (right >> 16) & 0xff;
				case 5:
					*(--op) = (right >> 24) & 0xff;
				case 4:
					*(--op) = left & 0xff;
				case 3:
					*(--op) = (left >> 8) & 0xff;
				case 2:
					*(--op) = (left >> 16) & 0xff;
				case 1:
					*(--op) = (left >> 24) & 0xff;
					break;
				}
				break;		/* we're done */
			}
		}
	}

	/*
	 * Done, return nothing.
	 */
	return 0;
}
