.SUFFIXES: .c .o
.c.o:
	${CC} -c ${ALL_CFLAGS} -o $@ $<

.SUFFIXES: .et
%.c %.h: %.et
	(cd $(dir $<); compile_et $(notdir $<))

.SUFFIXES: .ct
%.c: %.ct #*sigh*
	(cd $(dir $<); mk_cmds $(notdir $<))

.SUFFIXES: .lo
.c.lo:
	$(LIBTOOL) --mode=compile ${CC} -c -o $@ ${ALL_CFLAGS} $<

.PHONY: all
all: $(_BIN) $(_LSBIN) $(_LIBS) $(_SBIN)

.PHONY: check
check:

.PHONY: clean
clean: clean_common

.PHONY: clean_common
clean_common:
	$(LIBTOOL) --mode=clean $(RM) $(_BIN) $(_LSBIN) $(_SBIN) $(_LIBS) $(_LIBCLEAN)
	$(RM) $(_CLEAN)

.PHONY: install
install: install_common
install_common: $(_BIN) $(_LSBIN) $(_LIBS) $(_MAN1) $(_MAN8)
	$(LIBTOOL) --mode=install $(INSTALL) -m 755 $(_BIN) $(DESTDIR)$(bindir)
	$(LIBTOOL) --mode=install $(INSTALL) -m 755 $(_LSBIN) $(DESTDIR)$(lsbindir)
	$(LIBTOOL) --mode=install $(INSTALL) -m 755 $(_SBIN) $(DESTDIR)$(sbindir)
	$(LIBTOOL) --mode=install $(INSTALL) -m 644 $(_LIBS) $(DESTDIR)/$(libdir)
	$(INSTALL) -m 644 $(_MAN1) $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 $(_MAN8) $(DESTDIR)$(mandir)/man8
	$(INSTALL) -m 644 $(_DATA) $(DESTDIR)$(datadir)/zephyr
	$(INSTALL) -m 644 $(_SYSCONF) $(DESTDIR)$(sysconfdir)/zephyr
