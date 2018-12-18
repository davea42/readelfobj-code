#!/bin/sh

# Since git does not preserve ordering
# one will likely find this script
# useful to run from the top-level directory
# of readelfobj

if [ ! -d test ]
then
  echo "Do from readelfobj directory"
  exit 1
fi
if [ ! -d src ]
then
  echo "Do from readelfobj directory"
  exit 1
fi
if [ ! -d scripts ]
then
  echo "Do from readelfobj directory"
  exit 1
fi
if [ ! -d m4 ]
then
  echo "Do from readelfobj directory"
  exit 1
fi

t() {
  touch $1
  sleep 1
}

t Makefile.am
t src/Makefile.am
t test/Makefile.am
t configure.ac
t ltmain.sh
t m4/libtool.m4
t m4/ltoptions.m4
t m4/ltsugar.m4
t m4/ltversion.m4
t m4/lt~obsolete.m4
t autom4te.cache/output.3
t autom4te.cache/traces.3
t aclocal.m4
t configure
t autom4te.cache/output.1
t autom4te.cache/traces.1
t config.h.in
t autom4te.cache/requests
t compile
t config.guess
t config.sub
t install-sh
t missing
t INSTALL
t Makefile.in
t depcomp
t src/Makefile.in
t test/Makefile.in

