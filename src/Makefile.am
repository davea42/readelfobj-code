###Copyright (C) 2018 David Anderson
###This code is public domain and can be freely used or copied.

MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = readelfobj readobjmacho readobjpe object_detector

readelfobj_SOURCES =    \
dwarf_object_detector.h  \
dwarf_object_detector.c   \
dwarf_object_read_common.c \
dwarf_object_read_common.h \
dwarf_elf_naming.c   \
dwarf_elf_reloc_386.c \
dwarf_elf_reloc_386.h  \
dwarf_elf_reloc_mips.c	\
dwarf_elf_reloc_mips.h	 \
dwarf_elf_reloc_ppc.c    \
dwarf_elf_reloc_ppc.h    \
dwarf_elf_reloc_sparc.c	\
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
sanitized.c
readobjpe_CFLAGS = $(RO_CFLAGS_WARN)
readobjpe_CPPFLAGS = \
-I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/src



object_detector_SOURCES = object_detector_main.c \
   dwarf_object_detector.c \
   dwarf_object_detector.h
object_detector_CFLAGS = $(RO_CFLAGS_WARN) 

man_MANS = readelfobj.1

EXTRA_DIST = \
COPYING \
ChangeLog \
ChangeLog2018 \
NEWS \
README 

