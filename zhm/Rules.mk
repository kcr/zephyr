sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

ZHM_OBJS := $(d)/timer.o $(d)/queue.o $(d)/zhm.o $(d)/zhm_client.o $(d)/zhm_server.o

_LSBIN := $(_LSBIN) $(d)/zhm
_MAN8 := $(_MAN8) $(srcdir)/$(d)/zhm.8

_CLEAN := $(_CLEAN) $(ZHM_OBJS)

$(d)/zhm: ${ZHM_OBJS} ${LIBZEPHYR}
	${LIBTOOL} --mode=link ${CC} ${LDFLAGS} -o $@ ${ZHM_OBJS} ${LIBZEPHYR} ${HESIOD_LIBS}

${ZHM_OBJS}: $(d)/zhm.h $(d)/timer.h ${top_srcdir}/h/internal.h ${top_srcdir}/h/sysdep.h
${ZHM_OBJS}: ${BUILDTOP}/h/config.h ${BUILDTOP}/h/zephyr/zephyr.h
${ZHM_OBJS}: ${BUILDTOP}/h/zephyr/zephyr_err.h

d := $(dirstack_$(sp))
sp := $(basename $(sp))
