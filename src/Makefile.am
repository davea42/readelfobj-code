###Copyright (C) 2018 David Anderson
###This code is public domain and can be freely used or copied.

MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = readelfobj readobjmacho readobjpe object_detector

TESTS = $(check_PROGRAMS) $(check_TESTS)

readelfobj_SOURCES = \
dwarf_debuglink.c     \
dwarf_debuglink.h     \
dwarfstring.c         \
dwarfstring.h         \
dwarf_object_detector.h  \
dwarf_object_detector.c   \
dwarf_object_read_common.c \
dwarf_object_read_common.h  \
dwarf_elf_naming.c   \
dwarf_elf_reloc_386.c \
dwarf_elf_reloc_386.h  \
dwarf_elf_reloc_mips.c	\
dwarf_elf_reloc_mips.h	 \
dwarf_elf_reloc_ppc.c    \
dwarf_elf_reloc_ppc.h    \
dwarf_elf_reloc_sparc.c	 \
dwarf_elf_reloc_sparc.h	 \
dwarf_elf_reloc_arm.c    \
dwarf_elf_reloc_arm.h    \
dwarf_elf_reloc_aarch64.c \
dwarf_elf_reloc_aarch64.h \
dwarf_elf_reloc_ppc64.c	 \
dwarf_elf_reloc_ppc64.h	 \
dwarf_elf_reloc_x86_64.c \
dwarf_elf_reloc_x86_64.h \
dwarf_load_elf_headers.c \
errormessages.c \
readelfobj_version.h \
readelfobj.c \
dwarf_reading.h \
dw_elfstructs.h \
readelfobj.h \
sanitized.h \
sanitized.c

readelfobj_CFLAGS = $(RO_CFLAGS_WARN)
readelfobj_CPPFLAGS = \
-I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/src


readobjmacho_SOURCES =  readobjmacho.c \
sanitized.h \
dwarf_object_read_common.c \
dwarf_object_read_common.h \
dwarf_reading.h \
dwarf_object_detector.c \
dwarf_object_detector.h \
dwarf_machoread.h \
dwarf_machoread.c \
dwarf_macho_loader.h \
errormessages.c \
sanitized.c
readobjmacho_CFLAGS = $(RO_CFLAGS_WARN)
readobjmacho_CPPFLAGS = \
-I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/src

readobjpe_SOURCES =  readobjpe.c \
sanitized.h \
dwarf_object_read_common.c \
dwarf_object_read_common.h \
dwarf_reading.h \
dwarf_object_detector.c \
dwarf_object_detector.h \
dwarf_peread.h \
dwarf_peread.c \
dwarf_pe_descr.h \
errormessages.c \
sanitized.c
readobjpe_CFLAGS = $(RO_CFLAGS_WARN)
readobjpe_CPPFLAGS = \
-I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/src

object_detector_SOURCES = object_detector_main.c \
   dwarf_object_detector.c \
   errormessages.c \
   dwarf_object_detector.h
object_detector_CFLAGS = $(RO_CFLAGS_WARN) 

check_PROGRAMS = test_dwstring test_linkedtopath

test_dwstring_SOURCES = test_dwstring.c \
   dwarfstring.h dwarfstring.c 
test_dwstring_CFLAGS = $(RO_CFLAGS_WARN)
test_dwstring_CPPFLAGS = \
-I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/src

test_linkedtopath_SOURCES = test_linkedtopath.c \
   dwarfstring.h dwarfstring.c \
   dwarf_debuglink.h dwarf_debuglink.c \
   dwarf_object_read_common.c 
test_linkedtopath_CFLAGS = $(RO_CFLAGS_WARN) -DTESTING
test_linkedtopath_CPPFLAGS = \
-I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/src

check_TESTS = testdebuglink.sh

man_MANS = readelfobj.1

EXTRA_DIST = \
COPYING \
ChangeLog \
ChangeLog2018 \
NEWS \
README 

CLEANFILES = \
junk.ltp
