# README.md
Last update: August 2, 2024

The distribution consists entirely of C
source files (along with GNU configure scripts
to configure and build executables).
Builds and works on Linux, MacOS, and Windows msys2
and on across all of those the programs can read all the object
formats supported.

Requires python3 and a bash or sh shell.

## Contents
#### readelfobj

A program that reads elf object files and
prints the header information.  It also checks for certain
errors including wasted space in the object files.  It also
dumps Elf relocation sections, Elf symtab sections, and the
Elf dynamic section.

#### readobjpe

A program that reads PE object executables or
dlls and prints header information 
(mainly focused on sections that may be DWARF data).

#### readobjmacho

A program that reads MacOS dSYM files and
prints header information.
(mainly focused on sections that may be DWARF data).

#### object_detector

A program that reads a minimal set of
object header files (for any of the supported object types)
and prints basic information (file type, offset size, 
and endianness) about each file.
The only library used is standard C libc.

The code does not depend on any Elf, Mach-o, or Windows
libraries or headers, it just depends on a basic
POSIX/unix/Linux set of headers plus libc (or equivalent).

The test directory contains numerous examples of
using the programs.

To dump DWARF section contents one should use dwarfdump
(a part of the libdwarf-code github project) or
GNU readelf (in GNU binutils).

For Msys2 (Windows):
The configure option --enable-nonstandardprintf
may be required to avoid errors in certain printf.


## RUNNING MAKE CHECK from a release

Download the release from 

    https://github.com/davea42/readelfobj-code

and unpack

    #For example:
    tar xf readelfobj-0.1.0.xz
    cd  readelfobj-0.1.0 
    ./configure --enable-wall
    make
    make check
    #Building in a directory separate from
    #the download also works

## RUNNING MAKE CHECK from git clone:

Using a git clone is not recommended as
the autogen.sh step requires one have
the GNU autotools installed.
We build outside the source tree to keep the tree clean.


    #git clone https://github.com/davea42/readelfobj-code
    #Linux/Unix/MacOS/MinGW(Windows)
    src=/path/to/readelfobj-code
    cd $src
    sh autogen.sh

    bld=/tmp/robld 
    rm -rf $bld 
    mkdir $bld
    cd $bld
    # may not be necessary.
    sh $src/configure --enable-wall
    make check 

To update configure, update configure.ac and/or one or more
Makefile.am and do, in the top level directory:
  autoreconf -vif
You will need several GNU autotools installed for this
autoreconf to work.

