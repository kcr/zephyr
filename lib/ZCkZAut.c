/* This file is part of the Project Athena Zephyr Notification System.
 * It contains source for the ZCheckAuthentication function.
 *
 *	Created by:	Robert French
 *
 *	$Source$
 *	$Author$
 *
 *	Copyright (c) 1987,1991 by the Massachusetts Institute of Technology.
 *	For copying and distribution information, see the file
 *	"mit-copyright.h".
 */
/* $Header$ */

#ifndef lint
static const char rcsid_ZCheckAuthentication_c[] =
    "$Id$";
#endif

#include <internal.h>

/* Check authentication of the notice.
   If it looks authentic but fails the Kerberos check, return -1.
   If it looks authentic and passes the Kerberos check, return 1.
   If it doesn't look authentic, return 0

   When not using Kerberos, return true if the notice claims to be authentic.
   Only used by clients; the server uses its own routine.
 */
Code_t ZCheckZcodeAuthentication(ZNotice_t *notice,
				 struct sockaddr_in *from)
{
#ifdef HAVE_KRB5
    krb5_error_code result;
    krb5_creds *creds = NULL;
    krb5_keyblock *keyblock;
    krb5_enctype enctype;
    krb5_cksumtype cksumtype;
    krb5_data cksumbuf;
    int valid = 0;
    char *cksum0_base, *cksum1_base = NULL, *cksum2_base;
    char *x;
    unsigned char *asn1_data = NULL, *key_data;
    int asn1_len, key_len, cksum0_len = 0, cksum1_len = 0, cksum2_len = 0;

    cksumbuf.data = NULL;
#endif

    /* If the value is already known, return it. */
    if (notice->z_checked_auth != ZAUTH_UNSET)
        return (notice->z_checked_auth);

    if (!notice->z_auth)
        return (ZAUTH_NO);

    if (!notice->z_ascii_checksum)
        return (ZAUTH_NO);

    if (__Zephyr_authtype != ZAUTHTYPE_KRB5 &&
        __Zephyr_authtype != ZAUTHTYPE_KRB45) {
        return (notice->z_auth ? ZAUTH_YES : ZAUTH_NO);
    }
#ifdef HAVE_KRB5
    else { /* definite conditional compilation foul here.  later, krb5 will not
              be optional. */
        result = ZGetCreds(&creds);

        if (result != ZERR_NONE)
            valid = -1;

        if (result == ZERR_NONE) {
            /* Figure out what checksum type to use */
            keyblock = Z_credskey(creds);
            key_data = Z_keydata(keyblock);
            key_len = Z_keylen(keyblock);

            result = Z_ExtractEncCksum(keyblock, &enctype, &cksumtype);
        }
        if (result == ZERR_NONE) {
            /* Assemble the things to be checksummed */
            /* first part is from start of packet through z_default_format:
             * - z_version
             * - z_num_other_fields
             * - z_kind
             * - z_uid
             * - z_port
             * - z_auth
             * - z_authent_len
             * - z_ascii_authent
             * - z_class
             * - z_class_inst
             * - z_opcode
             * - z_sender
             * - z_recipient
             * - z_default_format
             */
            cksum0_base = notice->z_packet;
            x           = notice->z_default_format;
            cksum0_len  = x + strlen(x) + 1 - cksum0_base;
            /* second part is from z_multinotice through other fields:
             * - z_multinotice
             * - z_multiuid
             * - z_sender_(sock)addr
             * - z_charset
             * - z_other_fields[]
             */
            if (notice->z_num_hdr_fields > 15 ) {
                cksum1_base = notice->z_multinotice;
                if (notice->z_num_other_fields)
                    x = notice->z_other_fields[notice->z_num_other_fields - 1];
                else  {
                    /* see also server/kstuff.c:ZCheck{Realm,}Authentication */
                    /* XXXXXXXXXXXXXXXXXXXXXXX */
                    if (notice->z_num_hdr_fields > 16)
                        x = cksum1_base + strlen(cksum1_base) + 1; /* multinotice */
                    if (notice->z_num_hdr_fields > 17)
                        x = x + strlen(x) + 1; /* multiuid */
                    if (notice->z_num_hdr_fields > 18)
                        x = x + strlen(x) + 1; /* sender */
                }
                cksum1_len  = x + strlen(x) + 1 - cksum1_base; /* charset / extra field */
            }

            /* last part is the message body */
            cksum2_base = notice->z_message;
            cksum2_len  = notice->z_message_len;

            /* The following code checks for old-style checksums, which will go
               away once Kerberos 4 does. */
            if ((!notice->z_ascii_checksum || *notice->z_ascii_checksum != 'Z') &&
                key_len == 8 &&
                (enctype == ENCTYPE_DES_CBC_CRC ||
                 enctype == ENCTYPE_DES_CBC_MD4 ||
                 enctype == ENCTYPE_DES_CBC_MD5)) {
                /* try old-format checksum (covers cksum0 only) */

                ZChecksum_t our_checksum;

                our_checksum = z_quad_cksum((unsigned char *)cksum0_base, NULL, cksum0_len, 0,
                                            key_data);
                if (our_checksum == notice->z_checksum) {
                    result = ZERR_INTERNAL; /* kludge */
                    valid = 1;
                }
            }
        }

        if (result == ZERR_NONE) {
            cksumbuf.length = cksum0_len + cksum1_len + cksum2_len;
            cksumbuf.data = malloc(cksumbuf.length);
            if (!cksumbuf.data)
                result = ENOMEM;
        }

        if (result == ZERR_NONE) {
            memcpy(cksumbuf.data, cksum0_base, cksum0_len);
            if (cksum1_len)
                memcpy(cksumbuf.data + cksum0_len, cksum1_base, cksum1_len);
            memcpy(cksumbuf.data + cksum0_len + cksum1_len,
                   cksum2_base, cksum2_len);

            /* decode zcoded checksum */
            /* The encoded form is always longer than the original */
            asn1_len = strlen(notice->z_ascii_checksum) + 1;
            asn1_data = malloc(asn1_len);
            if (asn1_data == NULL)
                result = ENOMEM;
        }

        if (result == ZERR_NONE)
            result = ZReadZcode((unsigned char *)notice->z_ascii_checksum,
                                asn1_data, asn1_len, &asn1_len);

        if (result == ZERR_NONE)
            valid = Z_krb5_verify_cksum(keyblock, &cksumbuf, cksumtype,
                                        Z_KEYUSAGE_SRV_CKSUM, asn1_data, asn1_len);

        if (asn1_data != NULL)
            free(asn1_data);
        if (creds != NULL)
            krb5_free_creds(Z_krb5_ctx, creds);
        if (cksumbuf.data != NULL)
            free(cksumbuf.data);

        if (valid == 1)
            return ZAUTH_YES;
        else if (valid == -1)
            return ZAUTH_NO;
        else
            return ZAUTH_FAILED;
    }
#endif
}
