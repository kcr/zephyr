sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

_BIN := $(_BIN) $(d)/zleave
_MAN1 := $(_MAN1) $(srcdir)/$(d)/zleave.1
_CLEAN := $(_CLEAN) $(d)/zleave.o

$(d)/zleave: $(d)/zleave.o $(LIBZEPHYR)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(ALL_CFLAGS) -o $@ $< $(LIBZEPHYR) -lcom_err

$(d)/zleave: ${top_srcdir}/h/sysdep.h ${BUILDTOP}/h/config.h
$(d)/zleave: ${BUILDTOP}/h/zephyr/zephyr.h ${BUILDTOP}/h/zephyr/zephyr_err.h

d := $(dirstack_$(sp))
sp := $(basename $(sp))
