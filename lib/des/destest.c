/*
 * $Source: /srv/kcr/locker/zephyr/lib/des/Attic/destest.c,v $
 * $Author: ghudson $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 */

#include "mit-copyright.h"
#include "des.h"

#ifndef	lint
static const char rcsid_destest_c[] =
    "$Id: destest.c,v 1.3 1995-07-07 22:07:52 ghudson Exp $";
#endif

char clear[] = "eight bytes";
char cipher[8];
char key[8];
des_key_schedule schedule;

int main()
{
    int i;
    des_string_to_key("good morning!", key);
    i = des_key_sched(key, schedule);
    if (i) {
	printf("bad schedule (%d)\n", i);
	exit(1);
    }
    for (i = 1; i <= 10000; i++)
	des_ecb_encrypt(clear, cipher, schedule, i&1);
    return 0;
}
