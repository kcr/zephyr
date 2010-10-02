sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)


LIB_OBJS := $(d)/zephyr_err.lo $(d)/ZAsyncLocate.lo $(d)/ZCkAuth.lo \
	$(d)/ZCkIfNot.lo $(d)/ZClosePort.lo $(d)/ZCmpUID.lo $(d)/ZCmpUIDP.lo \
	$(d)/ZFlsLocs.lo $(d)/ZFlsSubs.lo $(d)/ZFmtAuth.lo $(d)/ZFmtList.lo \
	$(d)/ZFmtNotice.lo $(d)/ZFmtRaw.lo $(d)/ZFmtRawLst.lo \
	$(d)/ZFmtSmRLst.lo $(d)/ZFmtSmRaw.lo $(d)/ZFreeNot.lo \
	$(d)/ZGetLocs.lo $(d)/ZGetSender.lo $(d)/ZGetSubs.lo \
	$(d)/ZGetWGPort.lo $(d)/ZhmStat.lo $(d)/ZIfNotice.lo $(d)/ZInit.lo \
	$(d)/ZLocations.lo $(d)/ZMakeAscii.lo $(d)/ZMkAuth.lo \
	$(d)/ZNewLocU.lo $(d)/ZOpenPort.lo $(d)/ZParseNot.lo \
	$(d)/ZPeekIfNot.lo $(d)/ZPeekNot.lo $(d)/ZPeekPkt.lo $(d)/ZPending.lo \
	$(d)/ZReadAscii.lo $(d)/ZRecvNot.lo $(d)/ZRecvPkt.lo $(d)/ZRetSubs.lo \
	$(d)/ZSendList.lo $(d)/ZSendNot.lo $(d)/ZSendPkt.lo $(d)/ZSendRaw.lo \
	$(d)/ZSendRLst.lo $(d)/ZSetDest.lo $(d)/ZSetFD.lo $(d)/ZSetSrv.lo \
	$(d)/ZSubs.lo $(d)/ZVariables.lo $(d)/ZWait4Not.lo $(d)/Zinternal.lo \
	$(d)/ZMakeZcode.lo $(d)/ZReadZcode.lo $(d)/ZCkZAut.lo \
	$(d)/quad_cksum.lo $(d)/charset.lo $(d)/ZExpnRlm.lo

_LIBCLEAN := $(LIB_OBJS)

_LIBS := $(_LIBS) $(d)/libzephyr.la
_MAN1 := $(_MAN1) $(srcdir)/$(d)/zephyr.1
_CLEAN := $(d)/zephyr_err.[ch] ${BUILDTOP}/h/zephyr/zephyr_err.h
_INCLUDE := $(d)/zephyr_err.h

$(d)/libzephyr.la: ${LIB_OBJS}
	${LIBTOOL} --mode=link ${CC} -rpath ${libdir} -version-info 4:0:0 \
	  ${LDFLAGS} -o $@ ${LIB_OBJS} $(LIBZEPHYR_LIBS) -lcom_err $(LIBICONV)

all: ${BUILDTOP}/h/zephyr/zephyr_err.h
${BUILDTOP}/h/zephyr/zephyr_err.h: $(d)/zephyr_err.h
	cp -f $< ${BUILDTOP}/h/zephyr

check: check_$(d)
.PHONY: check_$(d)
check_$(d): d := $(d)
check_$(d):
	PYTHONPATH=${top_srcdir}/python python $(srcdir)/$(d)/zephyr_tests.py --builddir=$(BUILDTOP)
	PYTHONPATH=${top_srcdir}/python $(srcdir)/$(d)/zephyr_run_doctests --builddir=$(BUILDTOP)

${LIB_OBJS}: ${top_srcdir}/h/internal.h ${top_srcdir}/h/sysdep.h
${LIB_OBJS}: ${BUILDTOP}/h/config.h ${BUILDTOP}/h/zephyr/zephyr.h
${LIB_OBJS}: ${BUILDTOP}/h/zephyr/zephyr_err.h

d := $(dirstack_$(sp))
sp := $(basename $(sp))
