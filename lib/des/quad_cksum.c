/*
 * $Source: /srv/kcr/locker/zephyr/lib/des/Attic/quad_cksum.c,v $
 * $Author: ghudson $
 *
 * Copyright 1985, 1986, 1987, 1988 by the Massachusetts Institute
 * of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 *
 * Quadratic Congruential Manipulation Dectection Code
 *
 * ref: "Message Authentication"
 *		R.R. Jueneman, S. M. Matyas, C.H. Meyer
 *		IEEE Communications Magazine,
 *		Sept 1985 Vol 23 No 9 p 29-40
 *
 * This routine, part of the Athena DES library built for the Kerberos
 * authentication system, calculates a manipulation detection code for
 * a message.  It is a much faster alternative to the DES-checksum
 * method. No guarantees are offered for its security.	Refer to the
 * paper noted above for more information
 *
 * Implementation for 4.2bsd
 * by S.P. Miller	Project Athena/MIT
 */

/*
 * Algorithm (per paper):
 *		define:
 *		message to be composed of n m-bit blocks X1,...,Xn
 *		optional secret seed S in block X1
 *		MDC in block Xn+1
 *		prime modulus N
 *		accumulator Z
 *		initial (secret) value of accumulator C
 *		N, C, and S are known at both ends
 *		C and , optionally, S, are hidden from the end users
 *		then
 *			(read array references as subscripts over time)
 *			Z[0] = c;
 *			for i = 1...n
 *				Z[i] = (Z[i+1] + X[i])**2 modulo N
 *			X[n+1] = Z[n] = MDC
 *
 *		Then pick
 *			N = 2**31 -1
 *			m = 16
 *			iterate 4 times over plaintext, also use Zn
 *			from iteration j as seed for iteration j+1,
 *			total MDC is then a 128 bit array of the four
 *			Zn;
 *
 *			return the last Zn and optionally, all
 *			four as output args.
 *
 * Modifications:
 *	To inhibit brute force searches of the seed space, this
 *	implementation is modified to have
 *	Z	= 64 bit accumulator
 *	C	= 64 bit C seed
 *	N	= 2**63 - 1
 *  S	= S seed is not implemented here
 *	arithmetic is not quite real double integer precision, since we
 *	cant get at the carry or high order results from multiply,
 *	but nontheless is 64 bit arithmetic.
 */

#include "mit-copyright.h"
#include "des.h"
#include <netinet/in.h>

#ifndef	lint
static const char rcsid_quad_cksum_c[] =
    "$Id: quad_cksum.c,v 1.3 1995-07-07 22:07:59 ghudson Exp $";
#endif

/* Externals */
extern int des_debug;

/*** Routines ***************************************************** */

unsigned long
des_quad_cksum(in,out,length,out_count,c_seed)
    unsigned char *in;		/* input block */
    u_int32 *out;		/* optional longer output */
    long length;		/* original length in bytes */
    int out_count;		/* number of iterations */
    des_cblock c_seed;		/* secret seed, 8 bytes */
{

    /*
     * this routine both returns the low order of the final (last in
     * time) 32bits of the checksum, and if "out" is not a null
     * pointer, a longer version, up to entire 32 bytes of the
     * checksum is written unto the address pointed to.
     */

    register u_int32 z;
    register u_int32 z2;
    register u_int32 x;
    register u_int32 x2;
    register unsigned char *p;
    register long len;
    register int i;

    /* use all 8 bytes of seed */


    z = c_seed[0] +
	((u_int32)c_seed[1] << 8) +
	((u_int32)c_seed[2] << 16) +
	((u_int32)c_seed[3] << 24);

    z2 = c_seed[4] +
	 ((u_int32)c_seed[5] << 8) +
	 ((u_int32)c_seed[6] << 16) +
	 ((u_int32)c_seed[7] << 24);
    
    if (out == NULL)
	out_count = 1;		/* default */

    /* This is repeated n times!! */
    for (i = 1; i <=4 && i<= out_count; i++) {
	len = length;
	p = in;
	while (len) {
	    if (len > 1) {
		x = (z + p[0] + (p[1]<<8));
		p += 2;
		len -= 2;
	    }
	    else {
		x = (z + *(unsigned char *)p++);
		len = 0;
	    }
	    x2 = z2;
	    z  = ((x * x) + (x2 * x2)) % 0x7fffffff;
	    z2 = (x * (x2+83653421))   % 0x7fffffff; /* modulo */
	    if (des_debug & 8)
		printf("%lu %lu\n", (unsigned long) z, (unsigned long) z2);
	}

	if (out != NULL) {
	    *out++ = z;
	    *out++ = z2;
	}
    }
    /* return final z value as 32 bit version of checksum */
    return z;
}
