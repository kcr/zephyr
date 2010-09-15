sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

SERVER_OBJS := $(d)/zsrv_err.o $(d)/access.o $(d)/acl_files.o $(d)/bdump.o \
	$(d)/class.o $(d)/client.o $(d)/common.o $(d)/dispatch.o \
	$(d)/kstuff.o $(d)/main.o $(d)/server.o $(d)/subscr.o $(d)/timer.o \
	$(d)/uloc.o $(d)/zstring.o $(d)/realm.o $(d)/utf8proc.o

NEW_VERS := $(d)/new_vers.sh
VERSION_H := $(d)/version.h
VERSION_C := $(d)/version.c
VERSION_O := $(d)/version.o

_CLEAN := $(_CLEAN) $(SERVER_OBJS) $(d)/zsrv_err.[ch] $(VERSION_H) $(VERSION_O)
_SBIN := $(_SBIN) $(d)/zephyrd
_MAN8 := $(srcdir)/$(d)/zephyrd.8
_SYSCONF := $(srcdir)/$(d)/default.subscriptions


$(d)/zephyrd: ${SERVER_OBJS} ${LIBZEPHYR}
	sh $(NEW_VERS) > $(VERSION_H)
	$(CC) -c $(ALL_CFLAGS) -o $(VERSION_O) $(ALL_CFLAGS) $(VERSION_C)
	${LIBTOOL} --mode=link ${CC} ${LDFLAGS} -o $@ ${SERVER_OBJS} $(VERSION_O) \
		$(LIBZEPHYR) $(LIBS) -lcom_err ${HESIOD_LIBS}

$(d)/version.o: $(d)/version.h

$(d)/version.h: always
	(cd $(dir $@); ./new_vers.sh)

.PHONY: always
always:

${SERVER_OBJS}: $(d)/zserver.h $(d)/zsrv_err.h $(d)/timer.h $(d)/zsrv_conf.h $(d)/zstring.h $(d)/access.h $(d)/acl.h
${SERVER_OBJS}: ${top_srcdir}/h/internal.h ${top_srcdir}/h/sysdep.h
${SERVER_OBJS}: ${BUILDTOP}/h/config.h ${BUILDTOP}/h/zephyr/zephyr.h
${SERVER_OBJS}: ${BUILDTOP}/h/zephyr/zephyr_err.h

d := $(dirstack_$(sp))
sp := $(basename $(sp))
