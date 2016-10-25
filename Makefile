PREFIX=/usr
BIN_DIR=$(PREFIX)/bin
ZSH_FPATH=$(PREFIX)/share/zsh/site-functions

.PHONY: all
all: quoter

quoter: src/quoter.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o quoter src/quoter.c || \
	$(CC) -DAVOID_BUILTIN_EXPECT -DAVOID_ATTRIBUTE_NORETURN \
		$(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o quoter src/quoter.c

.PHONY: install
install: quoter
	install -D quoter $(DESTDIR)$(BIN_DIR)/quoter
	install -Dm 644 zsh/_quoter $(DESTDIR)/$(ZSH_FPATH)/_quoter

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)/$(BIN_DIR)/quoter
	rm -f $(DESTDIR)/$(ZSH_FPATH)/_quoter

.PHONY: clean
clean:
	rm -f ./quoter

.PHONY: distclean
distclean: clean
	rm -f ./quoter-*.asci ./quoter-*.tar.* ./quoter-*.zip

.PHONY: maintainer-clean
maintainer-clean: distclean
