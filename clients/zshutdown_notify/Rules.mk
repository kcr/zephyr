sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

_BIN := $(_BIN) $(d)/zshutdown_notify
_MAN8 := $(_MAN8) $(srcdir)/$(d)/zshutdown_notify.8
_CLEAN := $(_CLEAN) $(d)/zshutdown_notify.o

$(d)/zshutdown_notify: $(d)/zshutdown_notify.o $(LIBZEPHYR)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(ALL_CFLAGS) -o $@ $< $(LIBZEPHYR) -lcom_err

$(d)/zshutdown_notify: ${top_srcdir}/h/sysdep.h ${BUILDTOP}/h/config.h
$(d)/zshutdown_notify: ${BUILDTOP}/h/zephyr/zephyr.h ${BUILDTOP}/h/zephyr/zephyr_err.h

d := $(dirstack_$(sp))
sp := $(basename $(sp))
