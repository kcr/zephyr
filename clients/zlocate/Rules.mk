sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

_BIN := $(_BIN) $(d)/zlocate
_MAN1 := $(_MAN1) $(srcdir)/$(d)/zlocate.1
_CLEAN := $(_CLEAN) $(d)/zlocate.o

$(d)/zlocate: $(d)/zlocate.o $(LIBZEPHYR)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(ALL_CFLAGS) -o $@ $< $(LIBZEPHYR) -lcom_err

$(d)/zlocate: ${top_srcdir}/h/sysdep.h ${BUILDTOP}/h/config.h
$(d)/zlocate: ${BUILDTOP}/h/zephyr/zephyr.h ${BUILDTOP}/h/zephyr/zephyr_err.h

d := $(dirstack_$(sp))
sp := $(basename $(sp))
