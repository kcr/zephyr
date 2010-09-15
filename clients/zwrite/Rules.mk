sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

_BIN := $(_BIN) $(d)/zwrite
_MAN1 := $(_MAN1) $(srcdir)/$(d)/zwrite.1
_CLEAN := $(_CLEAN) $(d)/zwrite.o

$(d)/zwrite: $(d)/zwrite.o $(LIBZEPHYR)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(ALL_CFLAGS) -o $@ $< $(LIBZEPHYR) -lcom_err

$(d)/zwrite: ${top_srcdir}/h/sysdep.h ${BUILDTOP}/h/config.h
$(d)/zwrite: ${BUILDTOP}/h/zephyr/zephyr.h ${BUILDTOP}/h/zephyr/zephyr_err.h

d := $(dirstack_$(sp))
sp := $(basename $(sp))
