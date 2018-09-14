###Copyright (C) 2018 David Anderson
###This code is public domain and can be freely used or copied.

MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = readelfobj

readelfobj_SOURCES =  dynamic_section.c  \
elfem.c \
filedata.c \
generic_from_elf32.c  \
readelfobj_version.h \
readelfobj.c \
reading.h \
readelfobj.h \
sanitized.h \
sanitized.c

readelfobj_CFLAGS = $(RO_CFLAGS_WARN)

readelfobj_CPPFLAGS = \
-I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/src

man_MANS = readelfobj.1

EXTRA_DIST = \
COPYING \
ChangeLog \
NEWS \
README 

