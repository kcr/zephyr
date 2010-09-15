sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

_BIN := $(_BIN) $(d)/zaway
_MAN1 := $(_MAN1) $(srcdir)/$(d)/zaway.1
_CLEAN := $(_CLEAN) $(d)/zaway.o

$(d)/zaway: $(d)/zaway.o $(LIBZEPHYR)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(ALL_CFLAGS) -o $@ $< $(LIBZEPHYR) -lcom_err

$(d)/zaway: ${top_srcdir}/h/sysdep.h ${BUILDTOP}/h/config.h
$(d)/zaway: ${BUILDTOP}/h/zephyr/zephyr.h ${BUILDTOP}/h/zephyr/zephyr_err.h

d := $(dirstack_$(sp))
sp := $(basename $(sp))
