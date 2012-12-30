# Process this file with autoconf to produce a configure script.

# Copyright (C) 2012 Andrew Makousky
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

AC_INIT(morris-sim, 0.1)
AC_CONFIG_AUX_DIR(build-aux)
AC_CONFIG_HEADERS(config.h)
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_HEADER_STDC

# Configure GTK+.
pkg_modules="gtk+-2.0 >= 2.0.0"
PKG_CHECK_MODULES(GTK, [$pkg_modules])
PACKAGE_CFLAGS="$PACKAGE_CFLAGS $GTK_CFLAGS"
PACKAGE_LIBS="$PACKAGE_LIBS $GTK_LIBS"

win32=no
case "${host_os}" in
  *mingw* | pw32* | cygwin*)
	win32="yes"
    AC_CHECK_TOOL(RC, windres, :)
    if test "${RC}" = ":"; then
      AC_MSG_FAILURE([Error: you need windres to compile this program.])
    fi
    PACKAGE_CFLAGS="$PACKAGE_CFLAGS -mms-bitfields"
esac
AM_CONDITIONAL(WITH_WIN32, test x"$win32" = "xyes")
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

# Configure Gettext.
GETTEXT_PACKAGE=morris-sim
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

# Add the languages which your application supports here.
ALL_LINGUAS="es"

# Add maintainer information for the POT file here.
POT_COPYRIGHT_HOLDER="'Andrew Makousky'"
AC_SUBST(POT_COPYRIGHT_HOLDER)
POT_BUGS_ADDRESS="'andrew@localhost'"
AC_SUBST(POT_BUGS_ADDRESS)

AM_GLIB_GNU_GETTEXT

# Add optimization configuration.
AC_ARG_ENABLE(packed,
	AC_HELP_STRING([--disable-packed],
	[Disable array packing for parallel optimization.]),
	[if test $enableval = no; then
	   USE_PACKED=0
	 else
	   USE_PACKED=1
	 fi], [USE_PACKED=1])
AC_ARG_ENABLE(x86-asm,
	AC_HELP_STRING([--disable-x86-asm],
	[Disable inline x86 assembler.]),
	[if test $enableval = no; then
	   HAVE_X86_ASM=0
	 else
	   HAVE_X86_ASM=1
	 fi], [HAVE_X86_ASM=1])

if test $USE_PACKED -eq 1; then
  AC_DEFINE(USE_PACKED, 1,
  [Use tight array packing for parallel optimization.])
fi
if test $HAVE_X86_ASM -eq 1; then
  AC_DEFINE(HAVE_X86_ASM, 1,
  [Use inline x86 assembler.])
fi

AC_CONFIG_FILES([
Makefile
docs/Makefile
docs/reference/Makefile
po/Makefile.in
src/Makefile
])
AC_OUTPUT
