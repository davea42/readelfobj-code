###Copyright (C) 2018 David Anderson
###This code is public domain and can be freely used or copied.

MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = readobj

readobj_SOURCES =  dynamic_section.c  \
em.c \
filedata.c \
generic_from_elf32.c  \
readobj.c \
readobj.h \
sanitized.h \
sanitized.c

readobj_CFLAGS = $(RO_CFLAGS_WARN)

readobj_CPPFLAGS = \
-I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/src

EXTRA_DIST = \
COPYING \
ChangeLog \
NEWS \
README 

