# SPDX-License-Identifier: MIT
PREFIX=/usr
EPREFIX=
BINDIR=$(PREFIX)/bin
DATADIR=$(PREFIX)/share/quoter
ZSH_FPATH=$(PREFIX)/share/zsh/site-functions

.PHONY: FORCE all install uninstall clean distclean maintainer-clean

all: bin/quoter quoter_pipe.sh

quoter_pipe.sh:
	echo '#!$(EPREFIX)/bin/cat $(DATADIR)/quoter_pipe.sh' >quoter_pipe.sh

bin/quoter: src/quoter.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o bin/quoter src/quoter.c || \
	$(CC) -DAVOID_ATTRIBUTE_NORETURN \
		$(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o bin/quoter src/quoter.c || \
	$(CC) -DAVOID_BUILTIN_EXPECT \
		$(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o bin/quoter src/quoter.c || \
	$(CC) -DAVOID_BUILTIN_EXPECT -DAVOID_ATTRIBUTE_NORETURN \
		$(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o bin/quoter src/quoter.c

install: bin/quoter quoter_pipe.sh
	install -d '$(DESTDIR)$(BINDIR)'
	install -d '$(DESTDIR)/$(DATADIR)'
	install bin/quoter '$(DESTDIR)$(BINDIR)/quoter'
	install -m 755 quoter_pipe.sh '$(DESTDIR)$(BINDIR)/quoter_pipe.sh'
	install -m 644 bin/quoter_pipe.sh '$(DESTDIR)$(DATADIR)/quoter_pipe.sh'
	[ -z '$(ZSH_FPATH)' ] || install -d '$(DESTDIR)/$(ZSH_FPATH)'
	[ -z '$(ZSH_FPATH)' ] || install -m 644 zsh/_quoter '$(DESTDIR)/$(ZSH_FPATH)/_quoter'

uninstall: FORCE
	rm -f '$(DESTDIR)/$(BINDIR)/quoter'
	rm -f '$(DESTDIR)/$(BINDIR)/quoter_pipe.sh'
	rm -f '$(DESTDIR)/$(DATADIR)/quoter_pipe.sh'
	-rmdir '$(DESTDIR)/$(DATADIR)'
	[ -z '$(ZSH_FPATH)' ] || rm -f '$(DESTDIR)/$(ZSH_FPATH)/_quoter'
	-[ -z '$(ZSH_FPATH)' ] || rmdir '$(DESTDIR)/$(ZSH_FPATH)'

clean: FORCE
	rm -f bin/quoter src/quoter ./quoter ./quoter_pipe.sh \
		bin/*.o src/*.o *.o bin/*.obj src/*.obj ./*.obj

distclean: clean FORCE
	rm -f ./quoter-*.asc ./quoter-*.tar.* ./quoter-*.tar ./quoter-*.zip

maintainer-clean: distclean FORCE

check: bin/quoter
	sh ./testsuite

FORCE:
