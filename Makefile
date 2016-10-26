PREFIX=/usr
BIN_DIR=$(PREFIX)/bin
ZSH_FPATH=$(PREFIX)/share/zsh/site-functions

.PHONY: FORCE all install uninstall clean distclean maintainer-clean

all: bin/quoter

bin/quoter: src/quoter.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o bin/quoter src/quoter.c || \
	$(CC) -DAVOID_BUILTIN_EXPECT -DAVOID_ATTRIBUTE_NORETURN \
		$(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o bin/quoter src/quoter.c

install: bin/quoter
	install -D bin/quoter $(DESTDIR)$(BIN_DIR)/quoter
	install -Dm 644 bin/quoter_pipe.sh $(DESTDIR)$(BIN_DIR)/quoter_pipe.sh
	install -Dm 644 zsh/_quoter $(DESTDIR)/$(ZSH_FPATH)/_quoter

uninstall: FORCE
	rm -f $(DESTDIR)/$(BIN_DIR)/quoter
	rm -f $(DESTDIR)/$(BIN_DIR)/quoter_pipe.sh
	rm -f $(DESTDIR)/$(ZSH_FPATH)/_quoter

clean: FORCE
	rm -f bin/quoter src/quoter ./quoter \
		bin/*.o src/*.o *.o bin/*.obj src/*.obj ./*.obj

distclean: clean FORCE
	rm -f ./quoter-*.asc ./quoter-*.tar.* ./quoter-*.tar ./quoter-*.zip

maintainer-clean: distclean FORCE

FORCE:
