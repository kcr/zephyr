sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

ZCTL_OBJS := $(d)/zctl.o $(d)/zctl_cmds.o

_BIN := $(_BIN) $(d)/zctl
_MAN1 := $(_MAN1) $(srcdir)/$(d)/zctl.1
_CLEAN := $(_CLEAN) $(ZCTL_OBJS) $(d)/zctl_cmds.c

$(d)/zctl: ${ZCTL_OBJS} ${LIBZEPHYR}
	${LIBTOOL} --mode=link ${CC} ${LDFLAGS} -o $@ ${ZCTL_OBJS} \
		$(LIBZEPHYR) $(SS_LIBS) $(LIBS) -lcom_err

${ZCTL_OBJS}: ${top_srcdir}/h/sysdep.h ${BUILDTOP}/h/config.h
${ZCTL_OBJS}: ${BUILDTOP}/h/zephyr/zephyr.h ${BUILDTOP}/h/zephyr/zephyr_err.h

d := $(dirstack_$(sp))
sp := $(basename $(sp))
