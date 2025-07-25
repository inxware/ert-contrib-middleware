# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.63])

# Making point releases:
#   gdk_pixbuf_micro_version += 1;
#   gdk_pixbuf_interface_age += 1;
#   gdk_pixbuf_binary_age += 1;
#
# if any functions have been added, set gdk_pixbuf_interface_age to 0.
#
# if backwards compatibility has been broken:
#   gdk_pixbuf_major_version += 1;
#   gdk_pixbuf_interface_age = 0;
#   gdk_pixbuf_binary_age = 0;

m4_define([gdk_pixbuf_major_version], [2])
m4_define([gdk_pixbuf_minor_version], [22])
m4_define([gdk_pixbuf_micro_version], [0])
m4_define([gdk_pixbuf_interface_age], [0])
m4_define([gdk_pixbuf_binary_age],
          [m4_eval(100 * gdk_pixbuf_minor_version + gdk_pixbuf_micro_version)])
m4_define([gdk_pixbuf_version],
          [gdk_pixbuf_major_version.gdk_pixbuf_minor_version.gdk_pixbuf_micro_version])
m4_define([gdk_pixbuf_api_version], [2.0])
m4_define([gdk_pixbuf_binary_version], [2.10.0])


# required versions of other packages
m4_define([glib_required_version], [2.25.9])

AC_INIT([gdk-pixbuf], [gdk_pixbuf_version],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=gdk-pixbuf],
        [gdk-pixbuf])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([gdk-pixbuf/gdk-pixbuf.h])
AC_CONFIG_MACRO_DIR([m4])

# Save this value here, since automake will set cflags later
cflags_set=${CFLAGS+set}

AM_INIT_AUTOMAKE([1.10 foreign no-define -Wno-portability dist-bzip2])
AM_MAINTAINER_MODE([enable])

# Support silent build rules, requires at least automake-1.11. Enable
# by either passing --enable-silent-rules to configure or passing V=0
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([no])])

#
# For each of the libraries we build, we define the following
# substituted variables:
#
# foo_PACKAGES:     pkg-config packages this library requires
# foo_EXTRA_LIBS:   Libraries this module requires not pulled in by pkg-config
# foo_EXTRA_CFLAGS: cflags this module requires not pulled in by pkg-config
# foo_DEP_LIBS:     All libraries this module requires
# foo_DEP_CFLAGS:   All cflags this module requires

GDK_PIXBUF_MAJOR=gdk_pixbuf_major_version
GDK_PIXBUF_MINOR=gdk_pixbuf_minor_version
GDK_PIXBUF_MICRO=gdk_pixbuf_micro_version
GDK_PIXBUF_VERSION=gdk_pixbuf_version
GDK_PIXBUF_API_VERSION=gdk_pixbuf_api_version
GDK_PIXBUF_BINARY_VERSION=gdk_pixbuf_binary_version
AC_SUBST(GDK_PIXBUF_MAJOR)
AC_SUBST(GDK_PIXBUF_MINOR)
AC_SUBST(GDK_PIXBUF_MICRO)
AC_SUBST(GDK_PIXBUF_VERSION)
AC_SUBST(GDK_PIXBUF_API_VERSION)
AC_SUBST(GDK_PIXBUF_BINARY_VERSION)

# libtool versioning
#LT_RELEASE=$GTK_MAJOR_VERSION.$GTK_MINOR_VERSION
#LT_CURRENT=`expr $GTK_MICRO_VERSION - $GTK_INTERFACE_AGE`
#LT_REVISION=$GTK_INTERFACE_AGE
#LT_AGE=`expr $GTK_BINARY_AGE - $GTK_INTERFACE_AGE`
#LT_CURRENT_MINUS_AGE=`expr $LT_CURRENT - $LT_AGE`

m4_define([lt_current], [m4_eval(100 * gdk_pixbuf_minor_version + gdk_pixbuf_micro_version - gdk_pixbuf_interface_age)])
m4_define([lt_revision], [gdk_pixbuf_interface_age])
m4_define([lt_age], [m4_eval(gdk_pixbuf_binary_age - gdk_pixbuf_interface_age)])
LT_VERSION_INFO="lt_current:lt_revision:lt_age"
LT_CURRENT_MINUS_AGE=m4_eval(lt_current - lt_age)
AC_SUBST(LT_VERSION_INFO)
AC_SUBST(LT_CURRENT_MINUS_AGE)

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG

AC_CANONICAL_HOST

MATH_LIB=-lm
AC_MSG_CHECKING([for native Win32])
LIB_EXE_MACHINE_FLAG=X86
EXE_MANIFEST_ARCHITECTURE=X86
case "$host" in
  *-*-mingw*)
    os_win32=yes
    gio_can_sniff=no
    MATH_LIB=
    case "$host" in
    x86_64-*-*)
      LIB_EXE_MACHINE_FLAG=X64
      EXE_MANIFEST_ARCHITECTURE=AMD64
      ;;
    esac
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])

AC_SUBST(LIB_EXE_MACHINE_FLAG)
AC_SUBST(EXE_MANIFEST_ARCHITECTURE)

case $host in
  *-*-linux*)
    os_linux=yes
    ;;
esac

if test "$os_win32" = "yes"; then
  if test x$enable_static = xyes -o x$enable_static = x; then
    AC_MSG_WARN([Disabling static library build, must build as DLL on Windows.])
    enable_static=no
  fi
  if test x$enable_shared = xno; then
    AC_MSG_WARN([Enabling shared library build, must build as DLL on Windows.])
  fi
  enable_shared=yes
fi

dnl Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static win32-dll])
# Create libtool early, because it's used in configure
LT_OUTPUT

# Make sure we use 64-bit versions of various file stuff.
AC_SYS_LARGEFILE

AM_PROG_AS
AC_PATH_PROG(NM, nm, nm)

AC_MSG_CHECKING([for some Win32 platform])
AS_CASE(["$host"],
        [*-*-mingw*|*-*-cygwin*], [platform_win32=yes],
        [platform_win32=no]
)
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")
AM_CONDITIONAL(OS_UNIX, test "$os_win32" != "yes")
AM_CONDITIONAL(OS_LINUX, test "$os_linux" = "yes")

if test "$os_win32" = "yes"; then
  AC_CHECK_TOOL(WINDRES, windres, no)
  if test "$WINDRES" = no; then
    AC_MSG_ERROR([*** Could not find an implementation of windres in your PATH.])
  fi
  AC_CHECK_PROG(ms_librarian, lib.exe, yes, no)
fi
AM_CONDITIONAL(MS_LIB_AVAILABLE, test x$ms_librarian = xyes)

m4_define([debug_default],
          m4_if(m4_eval(gdk_pixbuf_minor_version % 2), [1], [yes], [minimum]))

dnl declare --enable-* args and collect ac_help strings
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
                             [turn on debugging @<:@default=debug_default@:>@]),,
              enable_debug=debug_default)
AC_ARG_ENABLE(rebuilds,
              [AC_HELP_STRING([--disable-rebuilds],
                              [disable all source autogeneration rules])],,
              [enable_rebuilds=yes])

AS_CASE([$enable_debug],
        [yes],
        [
          test "$cflags_set" = set || CFLAGS="$CFLAGS -g"
          GDK_PIXBUF_DEBUG_FLAGS="-DG_ENABLE_DEBUG -DG_ERRORCHECK_MUTEXES"
        ],

        [no], [GDK_PIXBUF_DEBUG_FLAGS="-DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"],

        [minimum], [GDK_PIXBUF_DEBUG_FLAGS="-DG_DISABLE_CAST_CHECKS"]
)

# Build time sanity check...
AM_SANITY_CHECK

changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac

  if test "x$enable_ansi" = "xyes"; then
    case " $CFLAGS " in
    *[\ \	]-ansi[\ \	]*) ;;
    *) CFLAGS="$CFLAGS -ansi" ;;
    esac

    case " $CFLAGS " in
    *[\ \	]-pedantic[\ \	]*) ;;
    *) CFLAGS="$CFLAGS -pedantic" ;;
    esac
  fi
fi
changequote([,])dnl

CPPFLAGS="$CPPFLAGS -DG_DISABLE_SINGLE_INCLUDES"

# Ensure MSVC-compatible struct packing convention is used when
# compiling for Win32 with gcc.
# What flag to depends on gcc version: gcc3 uses "-mms-bitfields", while
# gcc2 uses "-fnative-struct".
if test x"$os_win32" = xyes; then
  if test x"$GCC" = xyes; then
    msnative_struct=''
    AC_MSG_CHECKING([how to get MSVC-compatible struct packing])
    if test -z "$ac_cv_prog_CC"; then
      our_gcc="$CC"
    else
      our_gcc="$ac_cv_prog_CC"
    fi
    case `$our_gcc --version | sed -e 's,\..*,.,' -e q` in
      2.)
        if $our_gcc -v --help 2>/dev/null | grep fnative-struct >/dev/null; then
          msnative_struct='-fnative-struct'
        fi
        ;;
      *)
        if $our_gcc -v --help 2>/dev/null | grep ms-bitfields >/dev/null; then
          msnative_struct='-mms-bitfields'
        fi
        ;;
    esac
    if test x"$msnative_struct" = x ; then
      AC_MSG_RESULT([no way])
      AC_MSG_WARN([produced libraries might be incompatible with MSVC-compiled code])
    else
      CFLAGS="$CFLAGS $msnative_struct"
      AC_MSG_RESULT([${msnative_struct}])
    fi
  fi
fi

## Initial sanity check, done here so that users get told they
## have the wrong dependencies as early in the process as possible.
## Later on we actually use the cflags/libs from separate pkg-config
## calls. Oh, also the later pkg-config calls don't include
## the version requirements since those make the module lists
## annoying to construct
PKG_CHECK_MODULES(BASE_DEPENDENCIES, [glib-2.0 >= glib_required_version])

if test "$os_win32" != yes; then
    # libtool option to control which symbols are exported
    # right now, symbols starting with _ are not exported
    LIBTOOL_EXPORT_OPTIONS='-export-symbols-regex "^[[^_]].*"'
else
    # We currently use .def files on Windows (for gdk-pixbuf, gdk and gtk)
    LIBTOOL_EXPORT_OPTIONS=
fi
AC_SUBST(LIBTOOL_EXPORT_OPTIONS)

dnl ******************************************************
dnl * See whether to include shared library dependencies *
dnl ******************************************************

AC_ARG_ENABLE(explicit-deps,
              [AC_HELP_STRING([--enable-explicit-deps=@<:@yes/no/auto@:>@],
                              [use explicit dependencies in .pc files [default=auto]])],,
              [enable_explicit_deps=auto])

AC_MSG_CHECKING([Whether to write dependencies into .pc files])
case $enable_explicit_deps in
  auto)
    export SED
    deplibs_check_method=`(./libtool --config; echo 'eval echo \"$deplibs_check_method\"') | sh`
    if test "x$deplibs_check_method" '!=' xpass_all || test "x$enable_static" = xyes ; then
      enable_explicit_deps=yes
    else
      enable_explicit_deps=no
    fi
  ;;
  yes|no)
  ;;
  *) AC_MSG_ERROR([Value given to --enable-explicit-deps must be one of yes, no or auto])
  ;;
esac
AC_MSG_RESULT($enable_explicit_deps)

AM_CONDITIONAL(DISABLE_EXPLICIT_DEPS, test $enable_explicit_deps = no)

# define a MAINT-like variable REBUILD which is set if Perl
# and awk are found, so autogenerated sources can be rebuilt

AC_PATH_PROGS(PERL, perl5 perl)

REBUILD=\#
if test "x$enable_rebuilds" = "xyes" && \
     test -n "$PERL" && \
     $PERL -e 'exit !($] >= 5.002)' > /dev/null 2>&1 ; then
  REBUILD=
fi
AC_SUBST(REBUILD)

# sigsetjmp is a macro on some platforms, so AC_CHECK_FUNCS is not reliable
AC_MSG_CHECKING(for sigsetjmp)
AC_TRY_LINK([#include <setjmp.h>], [
sigjmp_buf env;
sigsetjmp(env, 0);
], gtk_ok=yes, gtk_ok=no)
AC_MSG_RESULT($gtk_ok)
if test "$gtk_ok" = "yes"; then
  AC_DEFINE(HAVE_SIGSETJMP, 1,
            [Define to 1 if sigsetjmp is available])
fi

# i18n stuff
AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE="$PACKAGE"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
                   [The prefix for our gettext translation domains.])

dnl The DU4 header files don't provide library prototypes unless
dnl -std1 is given to the native cc.
AC_MSG_CHECKING([for extra flags to get ANSI library prototypes])

gtk_save_LIBS=$LIBS
LIBS="$LIBS -lm"
AC_TRY_RUN([#include <math.h>
             int main (void) { return (log(1) != log(1.)); }],
     AC_MSG_RESULT(none needed),
     gtk_save_CFLAGS="$CFLAGS"
     CFLAGS="$CFLAGS -std1"
     AC_TRY_RUN([#include <math.h>
                 int main (void) { return (log(1) != log(1.)); }],
         AC_MSG_RESULT(-std1),
         AC_MSG_RESULT()
         CFLAGS="$gtk_save_CFLAGS"
         AC_MSG_WARN(
                [No ANSI prototypes found in library. (-std1 didn't work.)]),
         true
     ),
     AC_MSG_RESULT(none needed)
)
LIBS=$gtk_save_LIBS

AC_MSG_CHECKING(for the BeOS)
case $host in
  *-*-beos*)
    AC_MSG_RESULT(yes)
    MATH_LIB=
  ;;
  *)
    AC_MSG_RESULT(no)
  ;;
esac

AC_SUBST(MATH_LIB)
#
# see bug 162979
#
AC_MSG_CHECKING(for HP-UX)
case $host_os in
  hpux9* | hpux10* | hpux11*)
    AC_MSG_RESULT(yes)
    CFLAGS="$CFLAGS -DHPPEX -DSHMLINK"
  ;;
  *)
    AC_MSG_RESULT(no)
  ;;
esac

dnl NeXTStep cc seems to need this
AC_MSG_CHECKING([for extra flags for POSIX compliance])
AC_TRY_COMPILE([#include <dirent.h>], [DIR *dir;],
  AC_MSG_RESULT(none needed),
  gtk_save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -posix"
  AC_TRY_COMPILE([#include <dirent.h>], [DIR *dir;],
    AC_MSG_RESULT(-posix),
    AC_MSG_RESULT()
    CFLAGS="$gtk_save_CFLAGS"
    AC_MSG_WARN([Could not determine POSIX flag. (-posix didn't work.)])))

#
# Run AM_PATH_GLIB_2_0 to make sure that GLib is installed and working
#

GLIB_PACKAGES="gobject-2.0 gio-2.0 gmodule-no-export-2.0"

AM_PATH_GLIB_2_0(glib_required_version, :,
  AC_MSG_ERROR([
*** GLIB glib_required_version or better is required. The latest version of
*** GLIB is always available from ftp://ftp.gtk.org/pub/gtk/.]),
  gobject gmodule-no-export gthread)

# See if it's safe to turn G_DISABLE_DEPRECATED on.
GLIB_VERSION_MAJOR_MINOR=`$PKG_CONFIG --modversion glib-2.0 | sed "s/\.@<:@^.@:>@*\$//"`
GLIB_REQUIRED_VERSION_MAJOR_MINOR=`echo glib_required_version | sed "s/\.@<:@^.@:>@*\$//"`
if test "x$GLIB_VERSION_MAJOR_MINOR" = "x$GLIB_REQUIRED_VERSION_MAJOR_MINOR"; then
  CFLAGS="-DG_DISABLE_DEPRECATED $CFLAGS"
fi

CFLAGS="-DGDK_PIXBUF_DISABLE_DEPRECATED $CFLAGS"


dnl
dnl Check for bind_textdomain_codeset, including -lintl if GLib brings it in.
dnl
gtk_save_LIBS=$LIBS
LIBS="$LIBS $GLIB_LIBS"
AC_CHECK_FUNCS(bind_textdomain_codeset)
LIBS=$gtk_save_LIBS

AC_CHECK_HEADERS(unistd.h,
                 AC_DEFINE(HAVE_UNISTD_H, 1,
                           [Define to 1 if unistd.h is available]))

saved_cflags="$CFLAGS"
saved_ldflags="$LDFLAGS"


# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

##################################################
# Checks for gdk-pixbuf
##################################################

dnl ********************************************************
dnl * See whether we need to load our modules as .la files *
dnl ********************************************************

use_la_modules=false
case $host in
  *-aix*) use_la_modules=true
esac

if $use_la_modules ; then
   AC_DEFINE(USE_LA_MODULES, 1,
             [Whether to load modules via .la files rather than directly])
fi


AC_MSG_CHECKING(whether to build gmodulized gdk-pixbuf)

AC_ARG_ENABLE(modules,
              [AC_HELP_STRING([--disable-modules],
                              [disable dynamic module loading])])

dynworks=false
deps=
if test x$enable_modules = xno; then
    AC_MSG_RESULT(no)
else
    AC_MSG_RESULT(yes)
    AC_MSG_CHECKING(whether dynamic modules work)
    ## for loop is to strip newline
    tmp=`$PKG_CONFIG --variable=gmodule_supported gmodule-no-export-2.0`
    for I in $tmp; do
        dynworks=$I
    done

    dnl Now we check to see if our libtool supports shared lib deps
    dnl (in a rather ugly way even)
    if $dynworks; then
        pixbuf_libtool_config="${CONFIG_SHELL-/bin/sh} ./libtool --config"
        pixbuf_deplibs_check=`$pixbuf_libtool_config | \
            grep '^[[a-z_]]*check[[a-z_]]*_method=[['\''"]]' | \
            sed 's/.*[['\''"]]\(.*\)[['\''"]]$/\1/'`
        if test "x$pixbuf_deplibs_check" = "xnone" || \
           test "x$pixbuf_deplibs_check" = "xunknown" || \
           test "x$pixbuf_deplibs_check" = "x"; then
            dynworks=false
        fi
    fi

    if $dynworks; then
        AC_DEFINE(USE_GMODULE, 1,
                  [Define to 1 if gmodule works and should be used])
        AC_MSG_RESULT(yes)
    else
        AC_MSG_RESULT(no)
    fi
fi

dnl We allow people to disable image loaders explicitely, but if they don't we error
dnl out so that people don't accidentally build without them.

AC_ARG_WITH(libpng,
            [AC_HELP_STRING([--without-libpng],
                            [disable PNG loader for gdk-pixbuf])])
AC_ARG_WITH(libjpeg,
            [AC_HELP_STRING([--without-libjpeg],
                            [disable JPEG loader for gdk-pixbuf])])
AC_ARG_WITH(libtiff,
            [AC_HELP_STRING([--without-libtiff],
                            [disable TIFF loader for gdk-pixbuf])])
AC_ARG_WITH(libjasper,
            [AC_HELP_STRING([--with-libjasper],
                            [enable JPEG2000 loader for gdk-pixbuf])])

AC_ARG_ENABLE(gdiplus,
            [AC_HELP_STRING([--enable-gdiplus],
                            [enble GDI+ loaders for gdk-pixbuf (currently known to be broken)])],,
            [enable_gdiplus=no])

AM_CONDITIONAL(BUILD_GDIPLUS_LOADERS, [ test x$os_win32 = xyes && test x$enable_gdiplus != xno ])

dnl Test for libtiff
if test x$os_win32 = xno || test x$enable_gdiplus = xno; then
  if test x$with_libtiff != xno && test -z "$LIBTIFF"; then
    AC_CHECK_LIB(tiff, TIFFReadRGBAImageOriented,
      [AC_CHECK_HEADER(tiffio.h,
        TIFF='tiff'; LIBTIFF='-ltiff',
        AC_MSG_WARN(*** TIFF loader will not be built (TIFF header files not found) ***))],
      [AC_CHECK_LIB(tiff, TIFFWriteScanline,
        [AC_CHECK_HEADER(tiffio.h,
          TIFF='tiff'; LIBTIFF='-ltiff -ljpeg -lz',
          AC_MSG_WARN(*** TIFF loader will not be built (TIFF header files not found) ***))],
        [AC_CHECK_LIB(tiff34, TIFFFlushData,
          [AC_CHECK_HEADER(tiffio.h,
            TIFF='tiff'; LIBTIFF='-ltiff34 -ljpeg -lz',
            AC_MSG_WARN(*** TIFF loader will not be built (TIFF header files not found) ***))],
        AC_MSG_WARN(*** TIFF loader will not be built (TIFF library not found) ***), -ljpeg -lz -lm)], -ljpeg -lz -lm)], -lm)
  fi

  if test x$with_libtiff != xno && test -z "$LIBTIFF"; then
     AC_MSG_ERROR([
*** Checks for TIFF loader failed. You can build without it by passing
*** --without-libtiff to configure but some programs using GTK+ may
*** not work properly])
  fi
fi

dnl Test for libjpeg
if test x$os_win32 = xno || test x$enable_gdiplus = xno; then
  if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
    AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
      jpeg_ok=yes,
      jpeg_ok=no
      AC_MSG_WARN(*** JPEG loader will not be built (JPEG library not found) ***))
    if test "$jpeg_ok" = yes; then
      AC_MSG_CHECKING([for jpeglib.h])
      AC_TRY_CPP(
[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <jpeglib.h>],
        jpeg_ok=yes,
        jpeg_ok=no)
      AC_MSG_RESULT($jpeg_ok)
      if test "$jpeg_ok" = yes; then
        LIBJPEG='-ljpeg'
        AC_CHECK_LIB(jpeg, jpeg_simple_progression,
          AC_DEFINE(HAVE_PROGRESSIVE_JPEG, 1,
                    [Define to 1 is libjpeg supports progressive JPEG]),
          AC_MSG_WARN(JPEG library does not support progressive saving.))
      else
          AC_MSG_WARN(*** JPEG loader will not be built (JPEG header file not found) ***)
      fi
    fi
  fi

  if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
     AC_MSG_ERROR([
*** Checks for JPEG loader failed. You can build without it by passing
*** --without-libjpeg to configure but some programs using GTK+ may
*** not work properly])
  fi
fi

dnl Test for libpng
  if test x$with_libpng != xno && test -z "$LIBPNG"; then
    for l in libpng libpng14 libpng12 libpng13 libpng10 ; do
      AC_MSG_CHECKING(for $l)
      if $PKG_CONFIG --exists $l ; then
        AC_MSG_RESULT(yes)
        PNG='png'
        PNG_DEP_CFLAGS_PACKAGES=$l
        LIBPNG=`$PKG_CONFIG --libs $l`
        break
      else
        AC_MSG_RESULT(no)
      fi
    done
    if test x$PNG != xpng; then
      AC_MSG_RESULT(no)
      AC_CHECK_LIB(png, png_read_info,
        [AC_CHECK_HEADER(png.h,
          png_ok=yes,
          png_ok=no)],
        AC_MSG_WARN(*** PNG loader will not be built (PNG library not found) ***), -lz -lm)
      if test "$png_ok" = yes; then
        AC_MSG_CHECKING([for png_structp in png.h])
        AC_TRY_COMPILE([#include <png.h>],
          [png_structp pp; png_infop info; png_colorp cmap; png_create_read_struct;],
          png_ok=yes,
          png_ok=no)
        AC_MSG_RESULT($png_ok)
        if test "$png_ok" = yes; then
          PNG='png'; LIBPNG='-lpng -lz'
        else
          AC_MSG_WARN(*** PNG loader will not be built (PNG library is too old) ***)
        fi
      else
       AC_MSG_WARN(*** PNG loader will not be built (PNG header file not found) ***)
      fi
    fi
  fi

  if test x$with_libpng != xno && test -z "$LIBPNG"; then
     AC_MSG_ERROR([
*** Checks for PNG loader failed. You can build without it by passing
*** --without-libpng to configure but many programs using GTK+ will
*** not work properly. The PNG loader is also needed if you are compiling
*** from Git.])
  fi

dnl Test for libjasper
  if test x$with_libjasper = xyes && test -z "$LIBJASPER"; then
    AC_CHECK_LIB(jasper, jas_init, LIBJASPER=-ljasper, [], -ljpeg -lm)
  fi

  if test x$with_libjasper = xyes && test -z "$LIBJASPER"; then
     AC_MSG_ERROR([
*** Checks for JPEG2000 loader failed. You can build without it by passing
*** --without-libjasper to configure])
  fi

AC_SUBST(LIBTIFF)
AC_SUBST(LIBJPEG)
AC_SUBST(LIBPNG)
AC_SUBST(LIBJASPER)

AM_CONDITIONAL(BUILD_DYNAMIC_MODULES, $dynworks)

#
# Allow building some or all gdk-pixbuf loaders included
#
AC_MSG_CHECKING(pixbuf loaders to build)

dnl due to an autoconf bug, commas in the first arg to
dnl AC_HELP_STRING cause problems.
dnl AC_HELP_STRING([--with-included-loaders=LOADER1 LOADER2 ...],
dnl                [build the specified loaders into gdk-pixbuf])
AC_ARG_WITH(included_loaders,
[  --with-included-loaders=LOADER1,LOADER2,...
                          build the specified loaders into gdk-pixbuf])

if $dynworks; then
   :
else
   ## if the option was specified, leave it; otherwise disable included loaders
   if test x$with_included_loaders = xno; then
           with_included_loaders=yes
   fi
fi

# Use the traditional png loader instead of the GDI+ one on Windows,
# because some important apps like GIMP need to read and write
# arbitrary tEXt chunks which doesn't seem to be possible through GDI+

all_loaders="ani,icns,pcx,ras,tga,png,pnm,wbmp,xbm,xpm,qtif"
if test x$with_libjasper = xyes; then
  all_loaders="$all_loaders,jasper"
fi
if test x$os_win32 = xyes && test x$enable_gdiplus != xno; then
  # Skip PNG, see comment above
  gdip_formats="bmp emf gif ico jpeg tiff wmf"
  for f in $gdip_formats; do
    all_loaders="$all_loaders,gdip-$f"
  done
else
  all_loaders="$all_loaders,bmp,gif,ico,jpeg,tiff"
fi
included_loaders=""
# If no loaders specified, include all
if test "x$with_included_loaders" = xyes ; then
  included_loaders="$all_loaders"
else
  included_loaders="$with_included_loaders"
fi

AC_MSG_RESULT($included_loaders)

INCLUDED_LOADER_OBJ=
INCLUDED_LOADER_DEFINE=

IFS="${IFS=     }"; gtk_save_ifs="$IFS"; IFS=","
for loader in $included_loaders; do
 if echo "$all_loaders" | egrep "(^|,)$loader(\$|,)" > /dev/null; then
   :
 else
   AC_MSG_ERROR([the specified loader $loader does not exist])
 fi

 loader_underscores=`echo $loader | sed -e 's/-/_/g'`
 INCLUDED_LOADER_OBJ="$INCLUDED_LOADER_OBJ libstatic-pixbufloader-$loader.la"

 # Don't bother defining separate -DINCLUDE_gdip_foo for each gdip-foo loader
 case $loader in
   gdip-*) ;;
   *) INCLUDED_LOADER_DEFINE="$INCLUDED_LOADER_DEFINE -DINCLUDE_$loader_underscores";;
 esac
 eval INCLUDE_$loader_underscores=yes
done

# Just define one -DINCLUDE_gdiplus for all the gdip-foo loaders
# (except gdip-png, which isn't built at all)
if test x"$INCLUDE_gdip_ico" = xyes; then
  INCLUDED_LOADER_DEFINE="$INCLUDED_LOADER_DEFINE -DINCLUDE_gdiplus"
fi

IFS="$gtk_save_ifs"
AC_SUBST(INCLUDED_LOADER_OBJ)
AC_SUBST(INCLUDED_LOADER_DEFINE)

AM_CONDITIONAL(INCLUDE_PNG, [test x"$INCLUDE_png" = xyes])
AM_CONDITIONAL(INCLUDE_BMP, [test x"$INCLUDE_bmp" = xyes])
AM_CONDITIONAL(INCLUDE_WBMP, [test x"$INCLUDE_wbmp" = xyes])
AM_CONDITIONAL(INCLUDE_GIF, [test x"$INCLUDE_gif" = xyes])
AM_CONDITIONAL(INCLUDE_ICO, [test x"$INCLUDE_ico" = xyes])
AM_CONDITIONAL(INCLUDE_ANI, [test x"$INCLUDE_ani" = xyes])
AM_CONDITIONAL(INCLUDE_JPEG, [test x"$INCLUDE_jpeg" = xyes])
AM_CONDITIONAL(INCLUDE_PNM, [test x"$INCLUDE_pnm" = xyes])
AM_CONDITIONAL(INCLUDE_RAS, [test x"$INCLUDE_ras" = xyes])
AM_CONDITIONAL(INCLUDE_TIFF, [test x"$INCLUDE_tiff" = xyes])
AM_CONDITIONAL(INCLUDE_XPM, [test x"$INCLUDE_xpm" = xyes])
AM_CONDITIONAL(INCLUDE_XBM, [test x"$INCLUDE_xbm" = xyes])
AM_CONDITIONAL(INCLUDE_TGA, [test x"$INCLUDE_tga" = xyes])
AM_CONDITIONAL(INCLUDE_PCX, [test x"$INCLUDE_pcx" = xyes])
AM_CONDITIONAL(INCLUDE_ICNS, [test x"$INCLUDE_icns" = xyes])
AM_CONDITIONAL(INCLUDE_JASPER, [test x"$INCLUDE_jasper" = xyes])
AM_CONDITIONAL(INCLUDE_QTIF, [test x"$INCLUDE_qtif" = xyes])
# As all GDI+ loaders are either built-in or not, arbitrarily just
# check one of the variables here
AM_CONDITIONAL(INCLUDE_GDIPLUS, [test x"$INCLUDE_gdip_ico" = xyes])

if test x$gio_can_sniff = x; then
  AC_MSG_CHECKING([if gio can sniff png])
  gtk_save_LIBS="$LIBS"
  gtk_save_CFLAGS="$CFLAGS"
  LIBS="`$PKG_CONFIG --libs gio-2.0`"
  CFLAGS="`$PKG_CONFIG --cflags gio-2.0`"
  AC_RUN_IFELSE([AC_LANG_SOURCE([[
  #include <gio/gio.h>
  static const gsize data_size = 159;
  static const guint8 data[] =
  {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
    0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53, 0xde, 0x00, 0x00, 0x00,
    0x01, 0x73, 0x52, 0x47, 0x42, 0x00, 0xae, 0xce, 0x1c, 0xe9, 0x00, 0x00,
    0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0b, 0x13, 0x00, 0x00,
    0x0b, 0x13, 0x01, 0x00, 0x9a, 0x9c, 0x18, 0x00, 0x00, 0x00, 0x07, 0x74,
    0x49, 0x4d, 0x45, 0x07, 0xd8, 0x07, 0x0f, 0x10, 0x08, 0x15, 0x61, 0xd8,
    0x35, 0x37, 0x00, 0x00, 0x00, 0x19, 0x74, 0x45, 0x58, 0x74, 0x43, 0x6f,
    0x6d, 0x6d, 0x65, 0x6e, 0x74, 0x00, 0x43, 0x72, 0x65, 0x61, 0x74, 0x65,
    0x64, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x47, 0x49, 0x4d, 0x50, 0x57,
    0x81, 0x0e, 0x17, 0x00, 0x00, 0x00, 0x0c, 0x49, 0x44, 0x41, 0x54, 0x08,
    0xd7, 0x63, 0xf8, 0xff, 0xff, 0x3f, 0x00, 0x05, 0xfe, 0x02, 0xfe, 0xdc,
    0xcc, 0x59, 0xe7, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
    0x42, 0x60, 0x82
  };
  int
  main (int argc, char **argv)
  {
    char *content_type;
    char *image_png;
    g_type_init ();
    content_type = g_content_type_guess (NULL, data, data_size, NULL);
    image_png = g_content_type_from_mime_type ("image/png");
    return !!g_strcmp0 (content_type, image_png);
  }]])],
    [gio_can_sniff=yes
     AC_DEFINE(GDK_PIXBUF_USE_GIO_MIME, 1, [Define if gio can sniff image data])],
    [gio_can_sniff=no])
  AC_MSG_RESULT($gio_can_sniff)
  LIBS="$gtk_save_LIBS"
  CFLAGS="$gtk_save_CFLAGS"
fi

AM_CONDITIONAL(HAVE_TIFF, test "x$LIBTIFF" != x)
AM_CONDITIONAL(HAVE_PNG, test "x$LIBPNG" != x)
AM_CONDITIONAL(HAVE_JPEG, test "x$LIBJPEG" != x)
AM_CONDITIONAL(HAVE_JASPER, test "x$LIBJASPER" != x)

if $dynworks ; then
  STATIC_LIB_DEPS=
  if echo "$included_loaders" | egrep '(^|,)tiff($|,)' > /dev/null; then
    STATIC_LIB_DEPS="$STATIC_LIB_DEPS $LIBTIFF"
  fi
  if echo "$included_loaders" | egrep '(^|,)jpeg($|,)' > /dev/null; then
    STATIC_LIB_DEPS="$STATIC_LIB_DEPS $LIBJPEG"
  fi
  if echo "$included_loaders" | egrep '(^|,)png($|,)' > /dev/null; then
    STATIC_LIB_DEPS="$STATIC_LIB_DEPS $LIBPNG"
  fi
  if echo "$included_loaders" | egrep '(^|,)jasper($|,)' > /dev/null; then
    STATIC_LIB_DEPS="$STATIC_LIB_DEPS $LIBJASPER"
  fi
else
  STATIC_LIB_DEPS="$LIBTIFF $LIBJPEG $LIBPNG $LIBJASPER"
fi

# Checks to see whether we should include mediaLib
# support.
#
AC_CHECK_HEADER(sys/systeminfo.h,
                AC_DEFINE(HAVE_SYS_SYSTEMINFO_H, 1,
                          [Define to 1 if sys/systeminfo.h is available]))
AC_CHECK_HEADER(sys/sysinfo.h,
                AC_DEFINE(HAVE_SYS_SYSINFO_H, 1,
                          [Define to 1 if sys/sysinfo.h is available]))

AC_MSG_CHECKING(for mediaLib 2.3)
use_mlib25=no
# Check for a mediaLib 2.3 function since that is what the GTK+ mediaLib
# patch requires.
AC_CHECK_LIB(mlib, mlib_ImageSetStruct, use_mlib=yes, use_mlib=no)
if test $use_mlib = yes; then
    AC_DEFINE(USE_MEDIALIB, 1,
              [Define to 1 if medialib is available and should be used])
    MEDIA_LIB=-lmlib

    AC_MSG_CHECKING(for mediaLib 2.5)
    # Check for a mediaLib 2.5 function since that is what is needed for
    # gdk_rgb_convert integration.
    AC_CHECK_LIB(mlib, mlib_VideoColorRGBint_to_BGRAint, use_mlib25=yes, use_mlib25=no)
    if test $use_mlib25 = yes; then
        AC_DEFINE(USE_MEDIALIB25, 1,
                  [Define to 1 if medialib 2.5 is available])
    fi
fi
AM_CONDITIONAL(USE_MEDIALIB, test $use_mlib = yes)
AM_CONDITIONAL(USE_MEDIALIB25, test $use_mlib25 = yes)

# Checks to see if we should compile in MMX support (there will be
# a runtime test when the code is actually run to see if it should
# be used - this just checks if we can compile it.)
#
# This code is partially taken from Mesa
#
AC_MSG_CHECKING(for x86 platform)
case $host_cpu in
  i386|i486|i586|i686|i786|k6|k7)
        use_x86_asm=yes
        ;;
   *)
        use_x86_asm=no
esac
AC_MSG_RESULT($use_x86_asm)

use_mmx_asm=no
if test $use_x86_asm = yes; then
    save_ac_ext=$ac_ext
    ac_ext=S

    AC_MSG_CHECKING(compiler support for MMX)
    cp $srcdir/gdk-pixbuf/pixops/scale_line_22_33_mmx.S conftest.S
    if AC_TRY_EVAL(ac_compile); then
        use_mmx_asm=yes
    fi

    rm -rf conftest*

    ac_ext=$save_ac_ext
    if test $use_mmx_asm = yes; then
      AC_DEFINE(USE_MMX, 1,
                [Define to 1 if XXM is available and should be used])
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
    fi
fi

AM_CONDITIONAL(USE_MMX, test x$use_mmx_asm = xyes)

REBUILD_PNGS=
if test -z "$LIBPNG" && test x"$os_win32" = xno -o x$enable_gdiplus = xno; then
  REBUILD_PNGS=#
fi

dnl Look for a host system's gdk-pixbuf-csource if we are cross-compiling

AM_CONDITIONAL(CROSS_COMPILING, test $cross_compiling = yes)

if test $cross_compiling = yes; then
  AC_PATH_PROG(GDK_PIXBUF_CSOURCE, gdk-pixbuf-csource, no)
  AC_PATH_PROG(GTK_UPDATE_ICON_CACHE, gtk-update-icon-cache, no)
  if test x$GTK_UPDATE_ICON_CACHE = xno; then
    REBUILD_PNGS=#
  fi
fi

GDK_PIXBUF_PACKAGES="gmodule-no-export-2.0 gobject-2.0 gio-2.0"
GDK_PIXBUF_EXTRA_LIBS="$STATIC_LIB_DEPS $MATH_LIB $MEDIA_LIB"
GDK_PIXBUF_EXTRA_CFLAGS=
GDK_PIXBUF_DEP_LIBS="`$PKG_CONFIG --libs $GDK_PIXBUF_PACKAGES` $GDK_PIXBUF_EXTRA_LIBS"
GDK_PIXBUF_DEP_CFLAGS="`$PKG_CONFIG --cflags  gthread-2.0 $GDK_PIXBUF_PACKAGES $PNG_DEP_CFLAGS_PACKAGES` $GDK_PIXBUF_EXTRA_CFLAGS"

AC_SUBST(PNG_DEP_CFLAGS_PACKAGES)
AC_SUBST(GDK_PIXBUF_PACKAGES)
AC_SUBST(GDK_PIXBUF_EXTRA_LIBS)
AC_SUBST(GDK_PIXBUF_EXTRA_CFLAGS)
AC_SUBST(GDK_PIXBUF_DEP_LIBS)
AC_SUBST(GDK_PIXBUF_DEP_CFLAGS)

###############
# Check for X11
###############

GDK_PIXBUF_XLIB_PACKAGES=
GDK_PIXBUF_XLIB_EXTRA_CFLAGS=
GDK_PIXBUF_XLIB_EXTRA_LIBS=

if $PKG_CONFIG --exists x11; then
  AM_CONDITIONAL(USE_X11, true)
  PKG_CHECK_MODULES(GDK_PIXBUF_XLIB_DEP, x11 gobject-2.0)
else
  AM_CONDITIONAL(USE_X11, false)
fi

################################################################
# Strip -export-dynamic from the link lines of various libraries
################################################################

#
# pkg-config --libs gmodule includes the "export_dynamic" flag,
#  but this flag is only meaningful for executables. For libraries
#  the effect is undefined; what it causes on Linux is that the
#  export list from -export-symbols-regex is ignored and everything
#  is exported
#
# We are using gmodule-no-export now, but I'm leaving the stripping
# code in place for now, since pango and atk still require gmodule.
export SED
export_dynamic=`(./libtool --config; echo eval echo \\$export_dynamic_flag_spec) | sh`
if test -n "$export_dynamic"; then
  GDK_PIXBUF_DEP_LIBS=`echo $GDK_PIXBUF_DEP_LIBS | sed -e "s/$export_dynamic//"`
fi

##################################################
# GObject introspection
##################################################

GOBJECT_INTROSPECTION_CHECK([0.9.3])

##################################################
# Checks for gtk-doc and docbook-tools
##################################################

GTK_DOC_CHECK([1.11])

AC_CHECK_PROG(DB2HTML, db2html, true, false)
AM_CONDITIONAL(HAVE_DOCBOOK, $DB2HTML)

AC_ARG_ENABLE(man,
              [AC_HELP_STRING([--enable-man],
                              [regenerate man pages from Docbook [default=no]])],enable_man=yes,
              enable_man=no)

if test "${enable_man}" != no; then
  dnl
  dnl Check for xsltproc
  dnl
  AC_PATH_PROG([XSLTPROC], [xsltproc])
  if test -z "$XSLTPROC"; then
    enable_man=no
  fi
fi

AM_CONDITIONAL(ENABLE_MAN, test x$enable_man != xno)


##################################################
# Check for -Bsymbolic-functions linker flag used
# to avoid intra-library PLT jumps, if available.
##################################################

AC_ARG_ENABLE(Bsymbolic,
              [AC_HELP_STRING([--disable-Bsymbolic],
                              [avoid linking with -Bsymbolic])],,
              [SAVED_LDFLAGS="${LDFLAGS}"
               AC_MSG_CHECKING([for -Bsymbolic-functions linker flag])
               LDFLAGS=-Wl,-Bsymbolic-functions
               AC_TRY_LINK([], [int main (void) { return 0; }],
                           AC_MSG_RESULT(yes)
                           enable_Bsymbolic=yes,
                           AC_MSG_RESULT(no)
                           enable_Bsymbolic=no)
               LDFLAGS="${SAVED_LDFLAGS}"])

if test "x${enable_Bsymbolic}" == "xyes"; then
  GDK_PIXBUF_LINK_FLAGS=-Wl,-Bsymbolic-functions
fi

AC_SUBST(GDK_PIXBUF_LINK_FLAGS)


##################################################
# Output commands
##################################################

AC_CONFIG_FILES([
Makefile
gdk-pixbuf-2.0.pc
gdk-pixbuf-2.0-uninstalled.pc
gdk-pixbuf/Makefile
gdk-pixbuf/gdk_pixbuf.rc
gdk-pixbuf/gdk-pixbuf-features.h
gdk-pixbuf/pixops/Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/gdk-pixbuf/Makefile
docs/reference/gdk-pixbuf/version.xml
po/Makefile.in
tests/Makefile
contrib/Makefile
contrib/gdk-pixbuf-xlib/Makefile
contrib/gdk-pixbuf-xlib/gdk-pixbuf-xlib-2.0.pc
])

AC_OUTPUT
