sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

_BIN := $(_BIN) $(d)/znol
_MAN1 := $(_MAN1) $(srcdir)/$(d)/znol.1
_CLEAN := $(_CLEAN) $(d)/znol.o

$(d)/znol: $(d)/znol.o $(LIBZEPHYR)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) $(ALL_CFLAGS) -o $@ $< $(LIBZEPHYR) -lcom_err

$(d)/znol: ${top_srcdir}/h/sysdep.h ${BUILDTOP}/h/config.h
$(d)/znol: ${BUILDTOP}/h/zephyr/zephyr.h ${BUILDTOP}/h/zephyr/zephyr_err.h

d := $(dirstack_$(sp))
sp := $(basename $(sp))
