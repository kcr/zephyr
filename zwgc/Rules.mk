sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

INSTANTIATE :=./instantiate #XXX

ZWGC_OBJS := $(d)/port_dictionary.o $(d)/pointer_dictionary.o \
	$(d)/unsigned_long_dictionary.o $(d)/string_dictionary.o \
	$(d)/int_dictionary.o $(d)/string_dictionary_aux.o $(d)/parser.o \
	$(d)/lexer.o $(d)/node.o $(d)/exec.o $(d)/buffer.o $(d)/main.o \
	$(d)/zephyr.o $(d)/X_driver.o $(d)/substitute.o $(d)/port.o \
	$(d)/xshow.o $(d)/mux.o $(d)/eval.o $(d)/subscriptions.o \
	$(d)/notice.o $(d)/xcut.o $(d)/regexp.o $(d)/character_class.o \
	$(d)/text_operations.o $(d)/file.o $(d)/error.o $(d)/variables.o \
	$(d)/formatter.o $(d)/X_fonts.o $(d)/X_gram.o $(d)/tty_filter.o \
	$(d)/standard_ports.o $(d)/xselect.o $(d)/xmark.o $(d)/xrevstack.o \
	$(d)/xerror.o $(d)/new_string.o $(d)/new_memory.o $(d)/plus.o

_CLEAN := $(_CLEAN) $(ZWGC_OBJS) $(d)/port_dictionary.[ch] \
	$(d)/pointer_dictionary.[ch] $(d)/unsigned_long_dictionary.[ch] \
	$(d)/string_dictionary.[ch] $(d)/int_dictionary.[ch] \
	$(d)/char_stack.h $(d)/string_stack.h $(d)/xmode_stack.h \
	$(d)/y.tab.[ch]

_BIN := $(_BIN) $(d)/zwgc
_MAN1 := $(_MAN1) $(srcdir)/$(d)/zwgc.1
_DATA := $(_DATA) $(srcdir)/$(d)/zwgc.desc $(srcdir)/$(d)/zwgc_resources

$(d)/zwgc: $(ZWGC_OBJS) $(LIBZEPHYR)
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) -o $@ $(ZWGC_OBJS) \
		$(X_LIBS) $(LIBZEPHYR) $(LIBS) -lcom_err \
		$(ZWGC_LIBX11) $(X_EXTRA_LIBS) \
		$(TLIB) $(REGEX_LIBS) $(ARES_LIBS)

$(d)/port_dictionary.c $(d)/port_dictionary.h: $(d)/dictionary.c $(d)/dictionary.h
	(cd $(dir $@); ${INSTANTIATE} ${srcdir} dictionary port port.h)

$(d)/pointer_dictionary.c $(d)/pointer_dictionary.h: $(d)/dictionary.c $(d)/dictionary.h
	(cd $(dir $@); ${INSTANTIATE} ${srcdir} dictionary pointer pointer.h)

$(d)/unsigned_long_dictionary.c $(d)/unsigned_long_dictionary.h: $(d)/dictionary.c $(d)/dictionary.h
	(cd $(dir $@); ${INSTANTIATE} ${srcdir} dictionary unsigned_long unsigned_long.h)

$(d)/string_dictionary.c $(d)/string_dictionary.h: $(d)/dictionary.c $(d)/dictionary.h
	(cd $(dir $@); ${INSTANTIATE} ${srcdir} dictionary string new_string.h)

$(d)/int_dictionary.c $(d)/int_dictionary.h: $(d)/dictionary.c $(d)/dictionary.h
	(cd $(dir $@); ${INSTANTIATE} ${srcdir} dictionary int)

$(d)/char_stack.h: $(d)/stack.h
	(cd $(dir $@); ${INSTANTIATE} ${srcdir} stack char)

$(d)/string_stack.h: $(d)/stack.h
	(cd $(dir $@); ${INSTANTIATE} ${srcdir} stack string)

$(d)/xmode_stack.h: $(d)/stack.h
	(cd $(dir $@); ${INSTANTIATE} ${srcdir} stack xmode)

$(d)/lexer.o: $(d)/y.tab.h

$(d)/parser.o: $(d)/y.tab.c $(d)/y.tab.h
	${CC} -c ${ALL_CFLAGS} -o $@ $<

$(d)/y.tab.c $(d)/y.tab.h: ${srcdir}/$(d)/parser.y
	${YACC} ${YFLAGS} -b $(dir $@)/y $<

${ZWGC_OBJS}: ${top_srcdir}/h/sysdep.h ${BUILDTOP}/h/config.h
$(d)/zephyr.o: ${BUILDTOP}/h/zephyr/zephyr.h ${BUILDTOP}/h/zephyr/zephyr_err.h

$(d)/port_dictionary.o: $(d)/port.h $(d)/string_stack.h $(d)/new_string.h $(d)/new_memory.h
$(d)/pointer_dictionary.o: $(d)/pointer.h $(d)/new_string.h $(d)/new_memory.h
$(d)/unsigned_long_dictionary.o: $(d)/new_string.h $(d)/new_memory.h
$(d)/string_dictionary.o: $(d)/new_string.h $(d)/new_memory.h
$(d)/int_dictionary.o: $(d)/new_string.h $(d)/new_memory.h
$(d)/X_driver.o: $(d)/X_driver.h $(d)/new_memory.h $(d)/formatter.h $(d)/mux.h $(d)/variables.h $(d)/error.h
$(d)/X_driver.o: $(d)/X_gram.h $(d)/xselect.h $(d)/unsigned_long_dictionary.h
$(d)/X_fonts.o: $(d)/X_fonts.h $(d)/new_memory.h $(d)/new_string.h $(d)/error.h $(d)/pointer_dictionary.h
$(d)/X_fonts.o: $(d)/zwgc.h
$(d)/X_gram.o: $(d)/X_gram.h $(d)/xmark.h $(d)/zwgc.h $(d)/X_driver.h $(d)/X_fonts.h $(d)/error.h $(d)/new_string.h
$(d)/X_gram.o: $(d)/xrevstack.h $(d)/xerror.h $(d)/xselect.h
$(d)/browser.o: $(d)/zwgc.h
$(d)/buffer.o: $(d)/new_memory.h $(d)/buffer.h
$(d)/character_class.o: $(d)/character_class.h
$(d)/eval.o: $(d)/new_memory.h $(d)/node.h $(d)/eval.h $(d)/substitute.h $(d)/port.h $(d)/buffer.h $(d)/regexp.h
$(d)/eval.o: $(d)/text_operations.h $(d)/zwgc.h $(d)/variables.h
$(d)/exec.o: $(d)/new_memory.h $(d)/exec.h $(d)/eval.h $(d)/node.h $(d)/buffer.h $(d)/port.h $(d)/variables.h $(d)/notice.h
$(d)/file.o: $(d)/new_memory.h $(d)/new_string.h $(d)/error.h
$(d)/formatter.o: $(d)/new_memory.h $(d)/char_stack.h $(d)/string_dictionary.h $(d)/formatter.h
$(d)/formatter.o: $(d)/text_operations.h
$(d)/lexer.o: $(d)/new_memory.h $(d)/new_string.h $(d)/int_dictionary.h $(d)/lexer.h $(d)/parser.h
$(d)/main.o: $(d)/new_memory.h $(d)/zwgc.h $(d)/parser.h $(d)/node.h $(d)/exec.h $(d)/zephyr.h $(d)/notice.h
$(d)/main.o: $(d)/subscriptions.h $(d)/file.h $(d)/mux.h $(d)/port.h $(d)/variables.h $(d)/main.h
$(d)/mux.o: $(d)/mux.h $(d)/error.h $(d)/zwgc.h $(d)/pointer.h
$(d)/new_memory.o: $(d)/new_memory.h $(d)/int_dictionary.h
$(d)/new_string.o: $(d)/new_memory.h
$(d)/node.o: $(d)/new_memory.h $(d)/node.h
$(d)/notice.o: $(d)/new_memory.h $(d)/error.h $(d)/variables.h $(d)/notice.h
$(d)/port.o: $(d)/new_string.h $(d)/port_dictionary.h $(d)/port.h $(d)/notice.h $(d)/variables.h
$(d)/regexp.o: $(d)/regexp.h
$(d)/standard_ports.o: $(d)/new_memory.h $(d)/port.h $(d)/variables.h $(d)/error.h $(d)/main.h
$(d)/string_dictionary_aux.o: $(d)/new_memory.h $(d)/string_dictionary.h
$(d)/subscriptions.o: $(d)/new_memory.h $(d)/new_string.h $(d)/int_dictionary.h $(d)/zwgc.h
$(d)/subscriptions.o: $(d)/subscriptions.h $(d)/error.h $(d)/file.h $(d)/main.h
$(d)/substitute.o: $(d)/new_memory.h $(d)/lexer.h $(d)/substitute.h
$(d)/text_operations.o: $(d)/new_memory.h $(d)/text_operations.h $(d)/char_stack.h
$(d)/tty_filter.o: $(d)/new_memory.h $(d)/new_string.h $(d)/string_dictionary_aux.h $(d)/formatter.h
$(d)/tty_filter.o: $(d)/zwgc.h $(d)/error.h
$(d)/variables.o: $(d)/new_memory.h $(d)/notice.h $(d)/string_dictionary_aux.h $(d)/variables.h
$(d)/xcut.o: $(d)/new_memory.h $(d)/new_string.h $(d)/X_gram.h $(d)/zwgc.h $(d)/xselect.h $(d)/xmark.h $(d)/error.h
$(d)/xcut.o: $(d)/xrevstack.h
$(d)/xerror.o: $(d)/mux.h
$(d)/xmark.o: $(d)/X_gram.h $(d)/X_fonts.h $(d)/xmark.h $(d)/new_string.h
$(d)/xrevstack.o: $(d)/X_gram.h $(d)/zwgc.h
$(d)/xselect.o: $(d)/new_string.h $(d)/xselect.h
$(d)/xshow.o: $(d)/pointer_dictionary.h $(d)/new_memory.h $(d)/formatter.h $(d)/variables.h $(d)/zwgc.h
$(d)/xshow.o: $(d)/X_fonts.h $(d)/X_gram.h $(d)/xmode_stack.h
$(d)/zephyr.o: $(d)/new_string.h $(d)/zephyr.h $(d)/error.h $(d)/mux.h $(d)/subscriptions.h $(d)/variables.h
$(d)/zephyr.o: $(d)/pointer.h $(d)/X_driver.h

d := $(dirstack_$(sp))
sp := $(basename $(sp))
