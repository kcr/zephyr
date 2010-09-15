sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

_BIN := $(_BIN) $(d)/zstat
_MAN8 := $(_MAN1) $(srcdir)/$(d)/zstat.8
_CLEAN := $(_CLEAN) $(d)/zstat.o

$(d)/zstat: $(d)/zstat.o $(LIBZEPHYR)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(ALL_CFLAGS) -o $@ $< $(LIBZEPHYR) -lcom_err

$(d)/zstat: $(d)/zserver.h ${top_srcdir}/h/sysdep.h ${BUILDTOP}/h/config.h
$(d)/zstat: ${BUILDTOP}/h/zephyr/zephyr.h ${BUILDTOP}/h/zephyr/zephyr_err.h

d := $(dirstack_$(sp))
sp := $(basename $(sp))
