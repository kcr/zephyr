/*
 * This file is part of the Project Athena Zephyr Notification System.
 *
 * It contains declarations for use in the server, relating to access
 * control.
 *
 * Created by Ken Raeburn.
 *
 * $Source: /srv/kcr/locker/zephyr/server/access.h,v $
 * $Author: lwvanels $
 * $Id: access.h,v 1.3 1991-12-04 13:26:02 lwvanels Exp $
 *
 * Copyright (c) 1990 by the Massachusetts Institute of Technology.
 * For copying and distribution information, see the file
 * "mit-copyright.h".
 */

#include <zephyr/mit-copyright.h>

#include <zephyr/acl.h>
#include "zstring.h"
#include "unix.h"

typedef	enum _ZAccess_t {
	TRANSMIT,			/* use transmission acl */
	SUBSCRIBE,			/* use subscription acl */
	INSTWILD,			/* use instance wildcard acl */
	INSTUID				/* use instance UID identity acl */
} ZAccess_t;

typedef struct _ZAcl_t {
  char *acl_filename;
  int	acl_types;	/* Flag field indcating which acls
			   are present.  Used ONLY in access.c */
} ZAcl_t;

#ifdef __STDC__
# define        P(s) s
#else
# define P(s) ()
#endif

/* found in access.c */
extern void access_init P((void)), access_reinit P((void));

/* found in acl_files.c */
extern int acl_load P((char *));

#undef P

/* external data relevant */
extern int zdebug;
