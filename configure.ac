
###Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com>
###Derived from Torri's work, but this is by David Anderson
###This code is public domain and can be freely used or copied.

dnl defines the version name of  the libdwarf.so
m4_define([v_maj], [1])
m4_define([v_min], [0])
m4_define([v_mic], [0])
###m4_define([v_ver], [v_maj.v_min.v_mic])
###Returning to older version, .so.1
m4_define([v_ver], [v_maj])

m4_define([v_rel], [])
m4_define([lt_cur], [m4_eval(v_maj + v_min)])
m4_define([lt_rev], [v_mic])
m4_define([lt_age], [v_min])

### Sets the release name.
###m4_define([v_date], [m4_esyscmd_s([date "+%Y%m%d"])])
m4_define([v_date], [20181020])])

AC_PREREQ([2.52])
### 2nd arg to AC_INIT is the version 'number'.
AC_INIT([readelfobj], [v_date], [libdwarf-list -at- linuxmail -dot- org])
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
AC_CHECK_HEADERS([unistd.h elf.h malloc.h])

AC_CHECK_SIZEOF([short])
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([long long])
AS_IF( 
    [ test "${ac_cv_sizeof_short}" = "2" ],
    [ AC_DEFINE([DW_TYPEOF_16BIT],[short],[short is 16bit]) 
      type16bit="short" 
    ],
    [ AS_IF( [ test "${ac_cv_sizeof_int}" = "2" ],
         [AC_DEFINE([DW_TYPEOF_16BIT],[int],[int is 16bit])
             type16bit="int" 
         ],
         [AC_MSG_ERROR([We do not find a 16bit type])])
    ]
    )
AS_IF( 
    [ test "${ac_cv_sizeof_short}" = "4" ],
    [ AC_DEFINE([DW_TYPEOF_32BIT],[short],[short is 32bit]) 
      type32bit="short" 
    ],
    [ AS_IF( [ test "${ac_cv_sizeof_int}" = "4" ],
         [AC_DEFINE([DW_TYPEOF_32BIT],[int],[int is 32bit])
             type32bit="int" 
         ],
         [ AS_IF( [ test "${ac_cv_sizeof_long}" = "4" ],

            [AC_DEFINE([DW_TYPEOF_32BIT],[long],[long is 32bit])
             type32bit="long" 
            ],
            [AC_MSG_ERROR([We do not have a 32bit type])])
         ]
    )])

AS_IF(
    [ test "${ac_cv_sizeof_int}" = "8" ],
    [ AC_DEFINE([DW_TYPEOF_64BIT],[int],[int is 64bit]) 
      type64bit="int"
    ],
    [ AS_IF( [ test "${ac_cv_sizeof_long_long}" = "8" ],
         [AC_DEFINE([DW_TYPEOF_64BIT],[long long],
             [long long is 64bit])
             type64bit="long long" 
         ],
         [ AS_IF( [ test "${ac_cv_sizeof_long}" = "8" ],

            [AC_DEFINE([DW_TYPEOF_64BIT],[long],[long is 64bit])
             type64bit="long" 
            ],
            [AC_MSG_ERROR([We do not have a 64bit type])])
         ]
    )])

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
       cxx_compiler_flags="-Wall -Wextra -Wpointer-arith -Wmissing-declarations -Wcomment -Wformat -Wpedantic -Wuninitialized -Wshadow -Wno-long-long"

       c_compiler_flags="${cxx_compiler_flags} -Wmissing-prototypes -Wdeclaration-after-statement -Wbad-function-cast -Wmissing-parameter-type -Wnested-externs"
    ]
    ,
    [
       c_compiler_flags=""
       cxx_compiler_flags=""
    ]
    )

RO_CHECK_C_COMPILER_FLAGS([${c_compiler_flags}])

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

AC_C_BIGENDIAN
AC_CHECK_SIZEOF([unsigned long long])
AC_CHECK_SIZEOF([unsigned long])


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
echo "  Preferred 16bit......: ${type16bit}"
echo "  Preferred 32bit......: ${type32bit}"
echo "  Preferred 64bit......: ${type64bit}"
echo
echo "  readelfobj...........: always"
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

