###Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com>
###Derived from Torri's work, but this is by David Anderson
###This code is public domain and can be freely used or copied.

# dnl defines the version name of the programs
### to match next libdwarf release
m4_define([v_maj], [2])
m4_define([v_min], [3])
m4_define([v_mic], [0])
m4_define([v_ver], [v_maj.v_min.v_mic])

m4_define([v_rel], [])
m4_define([lt_cur], [m4_eval(v_maj + v_min)])
m4_define([lt_rev], [v_mic])
m4_define([lt_age], [v_min])


AC_PREREQ([2.52])
### 2nd arg to AC_INIT is the version 'number'.
AC_INIT([readelfobj], [v_ver], [libdwarf-list -at- linuxmail -dot- org])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([1.6 dist-xz])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
LT_INIT([win32-dll disable-shared static])

version_info="lt_cur:lt_rev:lt_age"
release_info="v_rel"
AC_SUBST([version_info])
AC_SUBST([release_info])

AC_PROG_CC
### MacOS does not have/use malloc.h
AC_CHECK_HEADERS([unistd.h elf.h malloc.h stddef.h stdint.h])

### for use in casts to uint to avoid 32bit warnings.
### Also needed by C++ cstdint
AC_TYPE_UINTPTR_T
AC_TYPE_INTPTR_T
### Now we know uintptr_t is either in stdint.h or
### is defined in config.h by configure.

AC_ARG_ENABLE([wall],
   [AS_HELP_STRING([--enable-wall],
                   [enable -Wall and other options @<:@default=no@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xyes"],
        [enable_wall="yes"],
        [enable_wall="no"])
   ],
   [enable_wall="no"])

AS_IF(
    [ test "x$enable_wall" = "xyes" ],
    [
       cxx_compiler_flags="-Wall -Wextra -Wpointer-arith -Wmissing-declarations -Wcomment -Wformat -Wpedantic -Wuninitialized -Wshadow -Wno-long-long -Werror"

       c_compiler_flags="${cxx_compiler_flags} -Wmissing-prototypes -Wdeclaration-after-statement -Wbad-function-cast -Wmissing-parameter-type -Wnested-externs -Werror"
    ]
    ,
    [
       c_compiler_flags=""
       cxx_compiler_flags=""
    ]
    )
AC_ARG_ENABLE([decompression],
   [AS_HELP_STRING([--enable-decompression],
                   [enable decompressing sections  @<:@default=yes@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xno"],
        [enable_decompression="no"],
        [enable_decompression="yes"])
   ],
   [enable_decompression="yes"])



AC_COMPILE_IFELSE(
 [AC_LANG_PROGRAM(
         [[
#ifdef HAVE_ELF_H
# include <elf.h>
#endif
         ]],
         [[
Elf64_Rela p;
p.r_offset = 1;
         ]])
    ],
    [
     AC_DEFINE([HAVE_ELF64_RELA], [1], [Set to 1 if Elf64_Rela defined in elf.h.])
     have_elf64_rela="yes"
    ],
    [have_elf64_rela="no"])

AC_MSG_CHECKING([for Elf64_Rela in elf.h])
AC_MSG_RESULT([${have_elf64_rela}])

AC_COMPILE_IFELSE(
 [AC_LANG_PROGRAM(
         [[
#ifdef HAVE_ELF_H
# include <elf.h>
#endif
         ]],
         [[
Elf64_Rel p;
Elf64_Sym s;
p.r_offset = 1;
s.st_name = 0;
         ]])
    ],
    [ AC_DEFINE([HAVE_ELF64_REL], [1], [Set to 1 if Elf64_Rel defined.])
     AC_DEFINE([HAVE_ELF64_SYM], [1], [Set to 1 if Elf64_Sym defined.])
     have_elf64_rel="yes"
     have_elf64_sym="yes"
    ],
    [have_elf64_rel="no"
     have_elf64_sym="no"])

AC_MSG_CHECKING([for Elf64_Rel in elf.h])
AC_MSG_RESULT([${have_elf64_rel}])

AC_C_BIGENDIAN([AC_DEFINE([WORDS_BIGENDIAN], [1], [Set to 1 if bigendian build])],,)
AC_CHECK_SIZEOF([unsigned long long])
AC_CHECK_SIZEOF([unsigned long])

# unused attribute
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
         [[
static unsigned int foo(unsigned int x, __attribute__ ((unused)) int y){
    unsigned int x2 = x + 1;
    return x2;
}
int goo() {
    unsigned int y = 0;
    y = foo(12, y);
}
         ]],
         [[
         ]])
    ],
    [
     have_unused="yes"
     AC_DEFINE(
         [HAVE_UNUSED_ATTRIBUTE], [1],
         [Set to 1 if __attribute__ ((unused)) is available.])
    ],
    [have_unused="no"])

AC_MSG_CHECKING([whether "unused" attribute is available])
AC_MSG_RESULT([${have_unused}])

AC_ARG_ENABLE(nonstandardprintf,AS_HELP_STRING([--enable-nonstandardprintf],
        [Use a special printf format for 64bit (default is NO)]),
        [ AC_DEFINE([HAVE_NONSTANDARD_PRINTF_64_FORMAT],[1],
           [Define 1 if need nonstandard printf format for 64bit] )
           [enable_nonstandardprintf="yes"]
        ],
        [enable_nonstandardprintf="no"])
AS_IF(
    [ test "x$enable_nonstandardprintf" = "xyes" ],
    [
       cxx_compiler_flags="${cxx_compiler_flags} -Wno-pedantic-ms-format"

       c_compiler_flags="${c_compiler_flags} -Wno-pedantic-ms-format"
    ]
    )
### Checks for libraries
requirements_libdwarf_pc=
AS_IF(
    [test "x${enable_decompression}" = "xyes"],
    [
        PKG_CHECK_MODULES([ZLIB], [zlib],
            [
             have_zlib="yes"
             have_pc_zlib="yes"
             requirements_pc=zlib
            ],
            [
             have_zlib="no"
             have_pc_zlib="no"
            ])
        PKG_CHECK_MODULES([ZSTD], [libzstd],
            [
             have_zstd="yes"
             have_pc_zstd="yes"
             requirements_pc="${requirements_pc} libzstd"
            ],
            [
             have_zstd="no"
             have_pc_zstd="no"
            ])
    ],
    [
        have_zstd="no"
        have_pc_zstd="no"
        have_zlib="no"
        have_pc_zlib="no"
    ])

AC_SUBST([requirements_pc])

AS_IF(
    [test "x${enable_decompression}" = "xyes"],
    [
        AS_IF(
            echo "dadebug zstd ${have_pc_zstd} ${have_zstd}"
            [test "x${have_pc_zstd}" = "xno"  -a "x${have_zstd}" = "xyes"],
            [
                AC_SEARCH_LIBS(
                [ZSTD_decompress], [zstd],
                [
                 ZSTD_LIBS="-lzstd"
                 echo "dadebug req set zstd"
                 requirements_libdwarf_libs="${requirements_libdwarf_libs} -lzstd"
                ],
                [have_zstd="no"])
            ])
    ],
    [
        have_zstd="no"
    ])

AS_IF(
    [test "x${have_zlib}" = "xyes"],
    [
     AC_DEFINE([HAVE_ZLIB], [1], [Set to 1 if zlib decompression is available.])
     AC_DEFINE([HAVE_ZLIB_H], [1], [Set to 1 if zlib.h header file is available.])
    ])

AS_IF(
    [test "x${have_zstd}" = "xyes"],
    [
     AC_DEFINE([HAVE_ZSTD], [1], [Set to 1 if zstd decompression is available.])
     AC_DEFINE([HAVE_ZSTD_H], [1], [Set to 1 if zstd.h header file is available.])
    ])
AC_SUBST([requirements_libdwarf_libs])

LIBS="$ZSTD_LIBS $ZLIB_LIBS"

RO_CHECK_C_COMPILER_FLAGS([${c_compiler_flags}])

AC_CONFIG_FILES([
Makefile
src/Makefile
test/Makefile
])

AC_OUTPUT

echo
echo "$PACKAGE $VERSION"
echo
echo "Configuration Options Summary:"
echo
echo "  BuildOS..............: ${build_os}"
echo "  HostOS...............: ${host_os}"
echo
echo "  Elf64_Rel............: ${have_elf64_rel}"
echo "  Elf64_Rela...........: ${have_elf64_rela}"
echo "  Elf64_Sym............: ${have_elf64_sym}"
echo "  sizeof unsigned long.: ${ac_cv_sizeof_unsigned_long}"
echo "  sizeof unsigned ll...: ${ac_cv_sizeof_unsigned_long_long}"
echo "  Elf64_Sym............: ${have_elf64_sym}"
echo "  Nonstandardprintf....: ${enable_nonstandardprintf}"
echo "  BuildOS-BigEndian....: ${ac_cv_c_bigendian}"
echo "  Elf zstd decompress..: ${have_zstd}"
echo "  Elf zlib decompress..: ${have_zlib}"
echo
echo "  readelfobj,readobjpe, "
echo "  readobjmacho, "
echo "  object_detector      : always"
echo
echo "Compilation............: make (or gmake)"
echo "  CPPFLAGS.............: $CPPFLAGS"
echo "  CFLAGS...............: $CFLAGS ${c_compiler_flags}"
echo "  LDFLAGS..............: $LDFLAGS"
echo "  LIBS.................: $LIBS"
echo
echo "Installation...........: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix...............: $prefix"
echo

