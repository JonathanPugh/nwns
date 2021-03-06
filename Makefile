MAKEFLAGS += --no-print-directory

PREFIX ?= /usr
SBINDIR ?= $(PREFIX)/sbin
MANDIR ?= $(PREFIX)/share/man
PKG_CONFIG ?= pkg-config

MKDIR ?= mkdir -p
INSTALL ?= install
REMOVE ?= rm
CC ?= "gcc"

CFLAGS ?= -O2 -g
CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration

OBJS = nwns.o netlist.o netops.o

OBJS += $(OBJS-y) $(OBJS-Y)

ALL=nwns


ifeq ($(NO_PKG_CONFIG),)
NL3xFOUND := $(shell $(PKG_CONFIG) --atleast-version=3.2 libnl-3.0 && echo Y)
ifneq ($(NL3xFOUND),Y)
NL31FOUND := $(shell $(PKG_CONFIG) --exact-version=3.1 libnl-3.1 && echo Y)
ifneq ($(NL31FOUND),Y)
NL3FOUND := $(shell $(PKG_CONFIG) --atleast-version=3 libnl-3.0 && echo Y)
ifneq ($(NL3FOUND),Y)
NL2FOUND := $(shell $(PKG_CONFIG) --atleast-version=2 libnl-2.0 && echo Y)
ifneq ($(NL2FOUND),Y)
NL1FOUND := $(shell $(PKG_CONFIG) --atleast-version=1 libnl-1 && echo Y)
endif
endif
endif
endif

ifeq ($(NL1FOUND),Y)
NLLIBNAME = libnl-1
endif

ifeq ($(NL2FOUND),Y)
CFLAGS += -DCONFIG_LIBNL20
LIBS += -lnl-genl
NLLIBNAME = libnl-2.0
endif

ifeq ($(NL3xFOUND),Y)
# libnl 3.2 might be found as 3.2 and 3.0
NL3FOUND = N
CFLAGS += -DCONFIG_LIBNL30
LIBS += -lnl-genl-3
NLLIBNAME = libnl-3.0
endif

ifeq ($(NL3FOUND),Y)
CFLAGS += -DCONFIG_LIBNL30
LIBS += -lnl-genl
NLLIBNAME = libnl-3.0
endif

# nl-3.1 has a broken libnl-gnl-3.1.pc file
# as show by pkg-config --debug --libs --cflags --exact-version=3.1 libnl-genl-3.1;echo $?
ifeq ($(NL31FOUND),Y)
CFLAGS += -DCONFIG_LIBNL30
LIBS += -lnl-genl
NLLIBNAME = libnl-3.1
endif

ifeq ($(NLLIBNAME),)
$(error Cannot find development files for any supported version of libnl)
endif

LIBS += $(shell $(PKG_CONFIG) --libs $(NLLIBNAME) ncurses)
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(NLLIBNAME) )
endif # NO_PKG_CONFIG

ifeq ($(V),1)
Q=
NQ=true
else
Q=@
NQ=echo
endif

all: $(ALL)

VERSION_OBJS := $(filter-out version.o, $(OBJS))

%.o: %.c @$(NQ) ' CC  ' $@
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

nwns:	$(OBJS)
	@$(NQ) ' CC  ' nwns
	$(Q)$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o nwns

check:
	$(Q)$(MAKE) all CC="REAL_CC=$(CC) CHECK=\"sparse -Wall\" gcc"

%.gz: %
	@$(NQ) ' GZIP' $<
	$(Q)gzip < $< > $@

install: nwns
	@$(NQ) ' INST nwns'
	$(Q)$(MKDIR) $(DESTDIR)$(SBINDIR)
	$(Q)$(INSTALL) -m 755 nwns $(DESTDIR)$(SBINDIR)
#	$(Q)$(MKDIR) $(DESTDIR)$(MANDIR)/man8/
#	$(Q)$(INSTALL) -m 644 nwns.8.gz $(DESTDIR)$(MANDIR)/man8/

remove: 
	@$(NQ) ' RM nwns'
	$(Q)$(REMOVE) $(SBINDIR)/nwns

clean:
	$(Q)rm -f nwns *.o *~ *.gz version.c *-stamp

