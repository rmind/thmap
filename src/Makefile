#
# This file is in the Public Domain.
#

PROJ=		thmap

SYSNAME:=	$(shell uname -s)
SYSARCH:=	$(shell uname -m)

CFLAGS+=	-std=c11 -O2 -g -Wall -Wextra -Werror
CFLAGS+=	-D_POSIX_C_SOURCE=200809L
CFLAGS+=	-D_GNU_SOURCE -D_DEFAULT_SOURCE

#
# Extended warning flags.
#
CFLAGS+=	-Wno-unknown-warning-option # gcc vs clang

CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith
CFLAGS+=	-Wmissing-declarations -Wredundant-decls -Wnested-externs
CFLAGS+=	-Wshadow -Wcast-qual -Wcast-align -Wwrite-strings
CFLAGS+=	-Wold-style-definition
CFLAGS+=	-Wsuggest-attribute=noreturn -Wjump-misses-init
CFLAGS+=	-Wduplicated-cond -Wmisleading-indentation -Wnull-dereference
CFLAGS+=	-Wduplicated-branches -Wrestrict

ifeq ($(MAKECMDGOALS),tests)
DEBUG=		1
endif

ifeq ($(DEBUG),1)
CFLAGS+=	-Og -DDEBUG -fno-omit-frame-pointer
ifeq ($(SYSARCH),x86_64)
CFLAGS+=	-fsanitize=address
LDFLAGS+=	-fsanitize=address
endif
else
CFLAGS+=	-DNDEBUG
endif

LIB=		lib$(PROJ)
INCS=		thmap.h
MANS=		thmap.3

OBJS=		thmap.o
OBJS+=		murmurhash.o

$(LIB).la:	LDFLAGS+=	-rpath $(LIBDIR) -version-info 1:0:0
install/%.la:	ILIBDIR=	$(DESTDIR)/$(LIBDIR)
install:	IINCDIR=	$(DESTDIR)/$(INCDIR)/
install:	IMANDIR=	$(DESTDIR)/$(MANDIR)/man3/

obj: $(OBJS)

lib: $(LIB).la

%.lo: %.c
	libtool --mode=compile --tag CC $(CC) $(CFLAGS) -c $<

$(LIB).la: $(shell echo $(OBJS) | sed 's/\.o/\.lo/g')
	libtool --mode=link --tag CC $(CC) $(LDFLAGS) -o $@ $(notdir $^)

install/%.la: %.la
	mkdir -p $(ILIBDIR)
	libtool --mode=install install -c $(notdir $@) $(ILIBDIR)/$(notdir $@)

install: $(addprefix install/,$(LIB).la)
	libtool --mode=finish $(LIBDIR)
	mkdir -p $(IINCDIR) && install -c $(INCS) $(IINCDIR)
	mkdir -p $(IMANDIR) && install -c $(MANS) $(IMANDIR)

tests: $(OBJS) t_$(PROJ).o
	$(CC) $(CFLAGS) $^ -o t_$(PROJ)
	MALLOC_CHECK_=3 ./t_$(PROJ)

stress: $(OBJS) t_stress.o murmurhash.o
	$(CC) $(CFLAGS) $^ -o t_stress -lpthread
	./t_stress

clean:
	libtool --mode=clean rm
	rm -rf .libs *.o *.lo *.la t_thmap t_stress

.PHONY: all obj lib install tests stress clean
