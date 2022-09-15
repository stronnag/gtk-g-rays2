#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT(gtk-g-rays2, 2.05)
AC_PREREQ(2.59)
AC_CONFIG_HEADER([config.h])
AC_CANONICAL_SYSTEM
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AM_INIT_AUTOMAKE([dist-xz tar-pax no-dist-gzip])

AC_SUBST(PACKAGE)
AC_SUBST(VERSION)

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET

# Platform
# Checks for libraries.

AC_PROG_INTLTOOL(0.23)

GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Define text domain.])

ALL_LINGUAS="en"
AM_GLIB_GNU_GETTEXT
AC_SUBST(ALL_LINGUAS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h libintl.h stdlib.h string.h sys/ioctl.h syslog.h termios.h unistd.h sys/wait.h signal.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MKTIME
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_CHECK_FUNCS([dup2 memset stpcpy strpbrk strtol])

AC_CHECK_LIB([bluetooth], [hci_open_dev], [], [AC_MSG_ERROR([alas, hci_open_dev was not found in libbluetooth])])

GLIB_REQUIRED=2.8.0
GTK_REQUIRED=0.2

AC_SUBST(GLIB_REQUIRED)
AC_SUBST(GTK_REQUIRED)
PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= ${GTK_REQUIRED} glib-2.0 gmodule-2.0 gudev-1.0 dbus-glib-1])

WBT201_CFLAGS="-Wall $GTK_CFLAGS"
WBT201_LIBS="$GTK_LIBS"


PKG_CHECK_MODULES(CAIRO,[cairo],
   have_cairo="YES",
   have_cairo="no")

if test "x${have_cairo}" != "xno" ; then
 AC_DEFINE(HAVE_CAIRO,[1],[Define to 1 if you have cairo libraries])
 if test "x${have_cairo}" = "xYES" ; then
   AC_DEFINE(NEED_CAIRO_H,[1],[Define to 1 if you need cairo.h])
   WBT201_LIBS="$WBT201_LIBS -lcairo"
 fi
fi

WBT201_LIBS="$WBT201_LIBS -lbluetooth -lm"

AC_SUBST(WBT201_CFLAGS)
AC_SUBST(WBT201_LIBS)

CFLAGS="$CFLAGS -D_GNU_SOURCE=1 -DGSEAL_ENABLE -Werror"

AC_OUTPUT(
Makefile
src/Makefile
data/Makefile
src/version.h
po/Makefile.in
po/Makefile
)
