#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
REQUIRED_AUTOMAKE_VERSION=1.8
PKG_NAME=gtk-g-rays2

(test -f $srcdir/configure.ac \
  && test -f $srcdir/src/wbt201.c) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

which gnome-autogen.sh || {
    echo "You need to install gnome-common from GNOME CVS"
    exit 1
}

USE_GNOME2_MACROS=1 . gnome-autogen.sh

