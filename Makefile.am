## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = avl httpp log net thread timing

lib_LTLIBRARIES = libigloo.la

pkginclude_HEADERS = \
    include/igloo/config.h \
    include/igloo/avl.h \
    include/igloo/encoding.h \
    include/igloo/httpp.h \
    include/igloo/igloo.h \
    include/igloo/log.h \
    include/igloo/resolver.h \
    include/igloo/sock.h \
    include/igloo/thread.h \
    include/igloo/timing.h \
    include/igloo/ro.h \
    include/igloo/types.h \
    include/igloo/typedef.h \
    include/igloo/interface.h \
    include/igloo/io.h \
    include/igloo/stdio.h \
    include/igloo/filter.h \
    include/igloo/objecthandler.h \
    include/igloo/logmsg.h \
    include/igloo/buffer.h \
    include/igloo/list.h \
    include/igloo/reportxml.h

libigloo_la_SOURCES = \
    src/private.c \
    src/libigloo.c \
    src/interface.c \
    src/ro.c \
    src/io.c \
    src/stdio.c \
    src/filter.c \
    src/objecthandler.c \
    src/logmsg.c \
    src/buffer.c \
    src/list.c \
    src/reportxml.c
libigloo_la_LIBADD = \
    avl/libiceavl.la \
    httpp/libicehttpp.la \
    log/libicelog.la \
    net/libicenet.la \
    thread/libicethread.la \
    timing/libicetiming.la

pkgconfig_DATA = igloo.pc

AM_CFLAGS = -I$(top_srcdir)/include

static:
	$(MAKE) all LDFLAGS="${LDFLAGS} -all-static"

include $(srcdir)/src/tests/Makefile.am
