dnl Process this file with autoconf to produce a configure script.

dnl Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009 Simon Josefsson
dnl
dnl This file is part of GNU Libidn.
dnl
dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_PREREQ(2.61)
AC_INIT([GNU Libidn], [1.16], [bug-libidn@gnu.org])
AC_COPYRIGHT([Copyright (c) 2002-2008 Simon Josefsson <simon@josefsson.org>.])

# Library code modified:                              REVISION++
# Interfaces changed/added/removed:   CURRENT++       REVISION=0
# Interfaces added:                             AGE++
# Interfaces removed:                           AGE=0
AC_SUBST(LT_CURRENT, 16)
AC_SUBST(LT_REVISION, 45)
AC_SUBST(LT_AGE, 5)

# For DEF file.
DLL_VERSION=`expr ${LT_CURRENT} - ${LT_AGE}`
AC_SUBST(DLL_VERSION)

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.10 -Wall -Werror -Wno-override])
AC_CONFIG_HEADERS(config.h)

# Checks for programs.
AC_LIBTOOL_WIN32_DLL
AC_PROG_CC
lgl_EARLY
gl_EARLY
AC_PROG_LIBTOOL
AM_MISSING_PROG(PERL, perl, $missing_dir)
AM_MISSING_PROG(HELP2MAN, help2man, $missing_dir)
AM_GNU_GETTEXT(external)
AM_GNU_GETTEXT_VERSION(0.17)
AM_ICONV
AM_PATH_LISPDIR

if test "$am_cv_func_iconv" != "yes"; then
  AC_MSG_NOTICE([
***

Your system does not have iconv().  The iconv() function is not
strictly required by the source code.  However, that may change in
the future, unless you report that this may be a problem for you.

Please report this to $PACKAGE_BUGREPORT.

***])
fi

# Checks for header files.
AC_HEADER_STDC

# For gnulib stuff in gl/.
gl_INIT
lgl_INIT

# Check if locale stuff works
AM_LANGINFO_CODESET

# Make it possible to disable TLD code.
AC_ARG_ENABLE(tld, AC_HELP_STRING([--disable-tld], [disable TLD code]),
		enable_tld=$enableval, enable_tld=yes)
AM_CONDITIONAL(TLD, test "$enable_tld" != "no")
if test "$enable_tld" != "no"; then
	AC_DEFINE([WITH_TLD], 1, [Define to 1 if you want TLD code.])
fi
AC_MSG_CHECKING([if TLD API for specific TLD rules should be built])
AC_MSG_RESULT($enable_tld)

# Check for gtk-doc.
GTK_DOC_CHECK(1.1)

# Check for java
AC_ARG_ENABLE(java, AC_HELP_STRING([--enable-java], [enable Java API]),,
		enable_java=no)
AM_PROG_GCJ
if test "$enable_java" != "no"; then
	AC_PROG_JAR
	AM_MISSING_PROG(GJDOC, gjdoc, $missing_dir)
fi
AM_CONDITIONAL(JAVA, test "$enable_java" != "no")
AC_MSG_CHECKING([if implementation in Java should be built])
AC_MSG_RESULT($enable_java)

# Check for C#
AM_CONDITIONAL(CSHARP, test -n "$HAVE_CSHARPCOMP")
AC_MSG_CHECKING([[if implementation in C# should be built]])
if test -n "$HAVE_CSHARPCOMP"; then
  enable_csharp=yes
else
  enable_csharp=no
fi
AC_MSG_RESULT($enable_csharp)

sj_VALGRIND
sj_UPDATE_HEADER_VERSION([$srcdir/lib/stringprep.h])

AC_ARG_ENABLE([gcc-warnings],
  [AS_HELP_STRING([--enable-gcc-warnings],
		  [turn on lots of GCC warnings (for developers)])],
  [case $enableval in
     yes|no) ;;
     *)      AC_MSG_ERROR([bad value $enableval for gcc-warnings option]) ;;
   esac
   gl_gcc_warnings=$enableval],
  [gl_gcc_warnings=no]
)

if test "$gl_gcc_warnings" = yes; then
  gl_WARN_ADD([-Werror], [WERROR_CFLAGS])
  gl_WARN_ADD([-Wframe-larger-than=80], [WSTACK_CFLAGS])

  nw="$nw -Wsystem-headers"         # Don't let system headers trigger warnings
  nw="$nw -Wc++-compat"             # We don't care strongly about C++ compilers
  nw="$nw -Woverlength-strings"     # Some of our strings are too large
  nw="$nw -Wsign-conversion"        # Too many warnings for now
  nw="$nw -Wconversion"             # Too many warnings for now
  nw="$nw -Wtraditional"            # Warns on #elif which we use often
  nw="$nw -Wtraditional-conversion" # Too many warnings for now
  nw="$nw -Wmissing-noreturn"       # Too many warnings for now
  nw="$nw -Wunreachable-code"       # Too many false positives
  nw="$nw -Wlogical-op"             # Too many false positives

  gl_MANYWARN_ALL_GCC([ws])
  gl_MANYWARN_COMPLEMENT(ws, [$ws], [$nw])
  for w in $ws; do
    gl_WARN_ADD([$w])
  done

  gl_WARN_ADD([-Wno-missing-field-initializers])
  gl_WARN_ADD([-fdiagnostics-show-option])
fi

AC_CONFIG_FILES([
  Makefile
  libidn.pc
  pom.xml
  csharp/Makefile
  contrib/doxygen/Doxyfile
  doc/Makefile
  doc/cyclo/Makefile
  doc/java/Makefile
  doc/reference/Makefile
  doc/specifications/Makefile
  doc/tld/Makefile
  examples/Makefile
  gl/Makefile
  gltests/Makefile
  java/Makefile
  java/gnu/Makefile
  java/gnu/inet/Makefile
  java/gnu/inet/encoding/Makefile
  java/misc/Makefile
  lib/Makefile
  lib/gl/Makefile
  lib/gltests/Makefile
  po/Makefile.in
  src/Makefile
  tests/Makefile
])
AC_OUTPUT

AC_MSG_NOTICE([summary of build options:

  version:          ${VERSION} shared $LT_CURRENT:$LT_REVISION:$LT_AGE
  Host type:        ${host}
  Install prefix:   ${prefix}
  Compiler:         ${CC}
  Warning flags:    ${WARN_CFLAGS}
  Library types:    Shared=${enable_shared}, Static=${enable_static}
  Valgrind:         ${VALGRIND}
  Version script:   $have_ld_version_script
  TLD code:         $enable_tld
  Build Java port:  $enable_java
  Build C# port:    $enable_csharp
])
