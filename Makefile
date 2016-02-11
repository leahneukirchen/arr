ALL=arr

CFLAGS=-g -O2 -Wall -Wextra -Wwrite-strings

DESTDIR=
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

all: $(ALL)

clean: FRC
	rm -f $(ALL)

install: FRC all
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m0755 $(ALL) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(ALL)

FRC:
