AC_PREREQ(2.60)

dnl please read gstreamer/docs/random/autotools before changing this file

dnl initialize autoconf
dnl releases only do -Wall, cvs and prerelease does -Werror too
dnl use a three digit version number for releases, and four for cvs/prerelease
AC_INIT(GStreamer Ugly Plug-ins, 0.10.17,
    http://bugzilla.gnome.org/enter_bug.cgi?product=GStreamer,
    gst-plugins-ugly)

AG_GST_INIT

dnl initialize automake
AM_INIT_AUTOMAKE([-Wno-portability 1.10])

dnl define PACKAGE_VERSION_* variables
AS_VERSION

dnl check if this is a release version
AS_NANO(GST_GIT="no", GST_GIT="yes")

dnl can autoconf find the source ?
AC_CONFIG_SRCDIR([ext/mad/gstmad.c])

dnl define the output header for config
AM_CONFIG_HEADER(config.h)

dnl AM_MAINTAINER_MODE only provides the option to configure to enable it
AM_MAINTAINER_MODE

dnl sets host_* variables
AC_CANONICAL_HOST

dnl use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
  [AM_DEFAULT_VERBOSITY=1
   AC_SUBST(AM_DEFAULT_VERBOSITY)])

dnl our libraries and install dirs use major.minor as a version
GST_MAJORMINOR=$PACKAGE_VERSION_MAJOR.$PACKAGE_VERSION_MINOR
dnl we override it here if we need to for the release candidate of new series
GST_MAJORMINOR=0.10
AC_SUBST(GST_MAJORMINOR)

dnl FIXME: this macro doesn't actually work;
dnl the generated libtool script has no support for the listed tags.
dnl So this needs to be fixed first if we want to use this
dnl AS_LIBTOOL_TAGS([CXX])

AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

dnl *** required versions of GStreamer stuff ***
GST_REQ=0.10.26
GSTPB_REQ=0.10.26

dnl *** autotools stuff ****

dnl allow for different autotools
AS_AUTOTOOLS_ALTERNATE

dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4 -I common/m4")

dnl set up gettext
dnl the version check needs to stay here because autopoint greps for it
AM_GNU_GETTEXT_VERSION(0.17)
AM_GNU_GETTEXT([external])
AG_GST_GETTEXT([gst-plugins-ugly-$GST_MAJORMINOR])

dnl *** check for arguments to configure ***

AG_GST_ARG_DEBUG
AG_GST_ARG_PROFILING
AG_GST_ARG_VALGRIND
AG_GST_ARG_GCOV

AG_GST_ARG_EXAMPLES

AG_GST_ARG_WITH_PKG_CONFIG_PATH
AG_GST_ARG_WITH_PACKAGE_NAME
AG_GST_ARG_WITH_PACKAGE_ORIGIN

AG_GST_ARG_WITH_PLUGINS

AG_GST_ARG_ENABLE_EXTERNAL

AG_GST_ARG_ENABLE_EXPERIMENTAL

dnl *** checks for platform ***

dnl * hardware/architecture *

dnl common/m4/gst-arch.m4
dnl check CPU type
AG_GST_ARCH

dnl Determine endianness
AC_C_BIGENDIAN

dnl *** checks for programs ***

dnl find a compiler
AC_PROG_CC
AC_PROG_CC_STDC

dnl determine c++ compiler
AC_PROG_CXX
dnl determine if c++ is available on this system
AC_CHECK_PROG(HAVE_CXX, $CXX, yes, no)

dnl check if the compiler supports '-c' and '-o' options
AM_PROG_CC_C_O

AC_PATH_PROG(VALGRIND_PATH, valgrind, no)
AM_CONDITIONAL(HAVE_VALGRIND, test ! "x$VALGRIND_PATH" = "xno")

dnl check if the compiler supports do while(0) macros
AG_GST_CHECK_DOWHILE_MACROS

dnl check for documentation tools
GTK_DOC_CHECK([1.3])
AS_PATH_PYTHON([2.1])
AG_GST_PLUGIN_DOCS([1.3],[2.1])

dnl *** checks for libraries ***

dnl check for libm, for sin()
AC_CHECK_LIBM
AC_SUBST(LIBM)

dnl *** checks for header files ***

dnl used by ext/a52dec
AX_CREATE_STDINT_H

dnl Check for malloc.h
AC_CHECK_HEADERS([malloc.h])

dnl *** checks for types/defines ***

dnl *** checks for structures ***

dnl *** checks for compiler characteristics ***

dnl *** checks for library functions ***

dnl Check for a way to display the function name in debug output
AG_GST_CHECK_FUNCTION

dnl *** checks for dependency libraries ***

dnl GLib is required
AG_GST_GLIB_CHECK([2.20])
ORC_CHECK([0.4.6])

dnl checks for gstreamer
dnl uninstalled is selected preferentially -- see pkg-config(1)
AG_GST_CHECK_GST($GST_MAJORMINOR, [$GST_REQ], yes)
AG_GST_CHECK_GST_BASE($GST_MAJORMINOR, [$GST_REQ], yes)
AG_GST_CHECK_GST_CHECK($GST_MAJORMINOR, [$GST_REQ], no)
AG_GST_CHECK_GST_PLUGINS_BASE($GST_MAJORMINOR, [$GSTPB_REQ], yes)
AM_CONDITIONAL(HAVE_GST_CHECK, test "x$HAVE_GST_CHECK" = "xyes")

dnl Check for documentation xrefs
GLIB_PREFIX="`$PKG_CONFIG --variable=prefix glib-2.0`"
GST_PREFIX="`$PKG_CONFIG --variable=prefix gstreamer-$GST_MAJORMINOR`"
GSTPB_PREFIX="`$PKG_CONFIG --variable=prefix gstreamer-plugins-base-$GST_MAJORMINOR`"
AC_SUBST(GLIB_PREFIX)
AC_SUBST(GST_PREFIX)
AC_SUBST(GSTPB_PREFIX)

dnl *** set variables based on configure arguments ***

dnl set license and copyright notice
GST_LICENSE="LGPL"
AC_DEFINE_UNQUOTED(GST_LICENSE, "$GST_LICENSE", [GStreamer license])
AC_SUBST(GST_LICENSE)

dnl set location of plugin directory
AG_GST_SET_PLUGINDIR

dnl set release date/time
AG_GST_SET_PACKAGE_RELEASE_DATETIME_WITH_NANO([$PACKAGE_VERSION_NANO],
  ["${srcdir}/gst-plugins-ugly.doap"],
  [$PACKAGE_VERSION_MAJOR.$PACKAGE_VERSION_MINOR.$PACKAGE_VERSION_MICRO])

dnl define an ERROR_CFLAGS Makefile variable
AG_GST_SET_ERROR_CFLAGS($GST_GIT, [
    -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls
    -Wwrite-strings -Wformat-nonliteral -Wformat-security -Wold-style-definition
    -Winit-self -Wmissing-include-dirs -Waddress
    -Waggregate-return -Wno-multichar -Wnested-externs ])

dnl define an ERROR_CXXFLAGS Makefile variable
AG_GST_SET_ERROR_CXXFLAGS($GST_GIT, [
    -Wmissing-declarations -Wredundant-decls
    -Wwrite-strings -Wformat-nonliteral -Wformat-security
    -Winit-self -Wmissing-include-dirs -Waddress -Waggregate-return
    -Wno-multichar])

dnl define correct level for debugging messages
AG_GST_SET_LEVEL_DEFAULT($GST_GIT)

dnl used in examples
AG_GST_DEFAULT_ELEMENTS

dnl *** plug-ins to include ***

dnl these are all the gst plug-ins, compilable without additional libs
AG_GST_CHECK_PLUGIN(asfdemux)
AG_GST_CHECK_PLUGIN(dvdlpcmdec)
AG_GST_CHECK_PLUGIN(dvdsub)
AG_GST_CHECK_PLUGIN(iec958)
AG_GST_CHECK_PLUGIN(mpegaudioparse)
AG_GST_CHECK_PLUGIN(mpegstream)
AG_GST_CHECK_PLUGIN(realmedia)
AG_GST_CHECK_PLUGIN(synaesthesia)

AC_CHECK_HEADERS([winsock2.h], HAVE_WINSOCK2_H=yes)

if test "x$HAVE_WINSOCK2_H" = "xyes"; then
  WIN32_LIBS="-lws2_32"
  AC_SUBST(WIN32_LIBS)
fi

dnl disable experimental plug-ins
if test "x$BUILD_EXPERIMENTAL" != "xyes"; then
  AG_GST_DISABLE_PLUGIN(synaesthesia)
fi

dnl *** ext plug-ins ***
dnl keep this list sorted alphabetically !

if test "x$BUILD_EXTERNAL" = "xyes"; then

echo
AC_MSG_NOTICE([Checking libraries for plugins in ext/])
echo

dnl *** a52dec ***
translit(dnm, m, l) AM_CONDITIONAL(USE_A52DEC, true)
AG_GST_CHECK_FEATURE(A52DEC, [a52dec], a52dec, [
  AC_CHECK_A52DEC(HAVE_A52DEC=yes, HAVE_A52DEC=no)
])

dnl *** amr-nb ***
translit(dnm, m, l) AM_CONDITIONAL(USE_AMRNB, true)
AG_GST_CHECK_FEATURE(AMRNB, [amrnb library], amrnb, [
  PKG_CHECK_MODULES(AMRNB, opencore-amrnb, HAVE_AMRNB="yes",
      [ AG_GST_CHECK_LIBHEADER(AMRNB, opencore-amrnb,
		           Decoder_Interface_init, $LIBM,
		           opencore-amrnb/interf_dec.h,
			   AMRNB_LIBS="-lopencore-amrnb")
      ])
  AC_SUBST(AMRNB_CFLAGS)
  AC_SUBST(AMRNB_LIBS)
])

dnl *** amr-wb dec ***
translit(dnm, m, l) AM_CONDITIONAL(USE_AMRWB, true)
AG_GST_CHECK_FEATURE(AMRWB, [amrwb library], amrwbdec, [
  PKG_CHECK_MODULES(AMRWB, opencore-amrwb, HAVE_AMRWB="yes",
      [ AG_GST_CHECK_LIBHEADER(AMRWB, opencore-amrwb,
			   D_IF_decode, ,
			   opencore-amrwb/dec_if.h,
			   AMRWB_LIBS="-lopencore-amrwb")
      ])
  AC_SUBST(AMRWB_CFLAGS)
  AC_SUBST(AMRWB_LIBS)
])

dnl *** cdio ***
translit(dnm, m, l) AM_CONDITIONAL(USE_CDIO, true)
AG_GST_CHECK_FEATURE(CDIO, [cdio library], cdio, [
  AG_GST_PKG_CHECK_MODULES(CDIO, libcdio >= 0.76)
])

dnl *** dvdread ***
translit(dnm, m, l) AM_CONDITIONAL(USE_DVDREAD, true)
AG_GST_CHECK_FEATURE(DVDREAD, [dvdread library], dvdreadsrc, [
  AG_GST_CHECK_LIBHEADER(DVDREAD, dvdread, DVDOpen, , dvdread/dvd_reader.h, DVDREAD_LIBS="-ldvdread")
  AC_SUBST(DVDREAD_LIBS)
])

dnl *** lame ***
translit(dnm, m, l) AM_CONDITIONAL(USE_LAME, true)
AG_GST_CHECK_FEATURE(LAME, [lame mp3 encoder library], lame, [
  AG_GST_CHECK_LIBHEADER(LAME, mp3lame, lame_init, $LIBM, lame/lame.h,
  [
    HAVE_LAME="yes"
    LAME_LIBS="-lmp3lame $LIBM"
    dnl is lame presets available
    LAME_CFLAGS=""
    AC_TRY_COMPILE([#include <lame/lame.h>], [ int preset = MEDIUM ],
      [LAME_CFLAGS="-DGSTLAME_PRESET"],
      [LAME_CFLAGS=""]
    )
    AC_TRY_COMPILE([#include <lame/lame.h>], [
        void *ptr = &lame_set_VBR_quality
      ],
      [LAME_CFLAGS="$LAME_CFLAGS -DHAVE_LAME_SET_VBR_QUALITY"],
      [LAME_CFLAGS="$LAME_CFLAGS"]
    )
  AC_SUBST(LAME_CFLAGS)
  AC_SUBST(LAME_LIBS)
  ])
])

dnl *** mad ***
translit(dnm, m, l) AM_CONDITIONAL(USE_MAD, true)
AG_GST_CHECK_FEATURE(MAD, [mad mp3 decoder], mad, [

  dnl check with pkg-config first
  AG_GST_PKG_CHECK_MODULES(ID3TAG, id3tag >= 0.15)
  if test "x$HAVE_ID3TAG" = "xno"; then
    dnl fall back to oldskool detection
    AC_CHECK_HEADER(id3tag.h, [
      save_LIBS=$LIBS
      LIBS="-lz"
      AC_CHECK_LIB(id3tag, id3_tag_options,
        HAVE_ID3TAG="yes" ID3TAG_LIBS="-lid3tag -lz")
      LIBS=$save_LIBS
      AC_SUBST(ID3TAG_LIBS)
    ])
  fi

  if test "x$HAVE_ID3TAG" = "xyes"; then
    dnl check with pkg-config first
    AG_GST_PKG_CHECK_MODULES(MAD, mad >= 0.15)
    MAD_LIBS="$MAD_LIBS $ID3TAG_LIBS"
    if test "x$HAVE_MAD" = "xno"; then
      dnl fall back to oldskool detection
      AC_CHECK_HEADER(mad.h, [
        AC_CHECK_LIB(mad, mad_decoder_finish,
          HAVE_MAD="yes" MAD_LIBS="-lmad $ID3TAG_LIBS")
      ])
    fi
  else
    HAVE_MAD="no"
    AC_MSG_WARN([libid3tag not available, cannot build MAD MP3 decoder plugin])
  fi
])
AC_SUBST(MAD_LIBS)

dnl *** mpeg2dec ***
translit(dnm, m, l) AM_CONDITIONAL(USE_MPEG2DEC, true)
AG_GST_CHECK_FEATURE(MPEG2DEC, [mpeg2dec], mpeg2dec, [
  AG_GST_PKG_CHECK_MODULES(MPEG2DEC, libmpeg2 >= 0.4.0)
])

dnl *** sidplay : works with libsidplay 1.36.x (not 2.x.x) ***
translit(dnm, m, l) AM_CONDITIONAL(USE_SIDPLAY, true)
AG_GST_CHECK_FEATURE(SIDPLAY, [libsidplay], sid, [
  GST_PATH_SIDPLAY()
])

dnl *** twolame ***
translit(dnm, m, l) AM_CONDITIONAL(USE_TWOLAME, true)
AG_GST_CHECK_FEATURE(TWOLAME, [twolame], twolame, [
  PKG_CHECK_MODULES(TWOLAME, twolame >= 0.3.10, [
      HAVE_TWOLAME="yes"], [
      HAVE_TWOLAME="no"
      AC_MSG_RESULT(no)
  ])
  AC_SUBST(TWOLAME_CFLAGS)
  AC_SUBST(TWOLAME_LIBS)
])

dnl *** x264 (MPEG-4 part 10/h.264/AVC encoder) ***
translit(dnm, m, l) AM_CONDITIONAL(USE_X264, true)
AG_GST_CHECK_FEATURE(X264, [x264 plug-in], x264, [
  AG_GST_PKG_CHECK_MODULES(X264, x264 >= 0.55.0)
])

else

dnl not building plugins with external dependencies,
dnl but we still need to set the conditionals

AM_CONDITIONAL(USE_A52DEC, false)
AM_CONDITIONAL(USE_AMRNB, false)
AM_CONDITIONAL(USE_AMRWB, false)
AM_CONDITIONAL(USE_CDIO, false)
AM_CONDITIONAL(USE_DVDREAD, false)
AM_CONDITIONAL(USE_LAME, false)
AM_CONDITIONAL(USE_MAD, false)
AM_CONDITIONAL(USE_MPEG2DEC, false)
AM_CONDITIONAL(USE_SIDPLAY, false)
AM_CONDITIONAL(USE_TWOLAME, false)
AM_CONDITIONAL(USE_X264, false)

fi dnl of EXT plugins

dnl *** finalize CFLAGS, LDFLAGS, LIBS

dnl Overview:
dnl GST_OPTION_CFLAGS:  common flags for profiling, debugging, errors, ...
dnl GST_*:              flags shared by built objects to link against GStreamer
dnl GST_ALL_LDFLAGS:    linker flags shared by all
dnl GST_LIB_LDFLAGS:    additional linker flags for all libaries
dnl GST_LT_LDFLAGS:     library versioning of our libraries
dnl GST_PLUGIN_LDFLAGS: flags to be used for all plugins

dnl GST_OPTION_CFLAGS
if test "x$USE_DEBUG" = xyes; then
   PROFILE_CFLAGS="-g"
fi
AC_SUBST(PROFILE_CFLAGS)

if test "x$PACKAGE_VERSION_NANO" = "x1"; then
  dnl Define _only_ during CVS (not pre-releases or releases)
  DEPRECATED_CFLAGS="-DGST_DISABLE_DEPRECATED"
else
  DEPRECATED_CFLAGS=""
fi
AC_SUBST(DEPRECATED_CFLAGS)

dnl every flag in GST_OPTION_CFLAGS and GST_OPTION_CXXFLAGS can be overridden
dnl at make time with e.g. make ERROR_CFLAGS=""
GST_OPTION_CFLAGS="\$(WARNING_CFLAGS) \$(ERROR_CFLAGS) \$(DEBUG_CFLAGS) \$(PROFILE_CFLAGS) \$(GCOV_CFLAGS) \$(OPT_CFLAGS) \$(DEPRECATED_CFLAGS)"
GST_OPTION_CXXFLAGS="\$(WARNING_CXXFLAGS) \$(ERROR_CXXFLAGS) \$(DEBUG_CFLAGS) \$(PROFILE_CFLAGS) \$(GCOV_CFLAGS) \$(OPT_CFLAGS) \$(DEPRECATED_CFLAGS)"
AC_SUBST(GST_OPTION_CFLAGS)
AC_SUBST(GST_OPTION_CXXFLAGS)

dnl FIXME: do we want to rename to GST_ALL_* ?
dnl prefer internal headers to already installed ones
dnl also add builddir include for enumtypes and marshal
dnl add GST_OPTION_CFLAGS, but overridable
GST_CXXFLAGS="-I\$(top_srcdir)/gst-libs -I\$(top_builddir)/gst-libs $GST_CFLAGS $GLIB_EXTRA_CFLAGS \$(GST_OPTION_CXXFLAGS)"
GST_CFLAGS="-I\$(top_srcdir)/gst-libs -I\$(top_builddir)/gst-libs $GST_CFLAGS $GLIB_EXTRA_CFLAGS \$(GST_OPTION_CFLAGS)"
AC_SUBST(GST_CFLAGS)
AC_SUBST(GST_CXXFLAGS)
AC_SUBST(GST_LIBS)

dnl LDFLAGS really should only contain flags, not libs - they get added before
dnl whatevertarget_LIBS and -L flags here affect the rest of the linking
GST_ALL_LDFLAGS="-no-undefined"
AC_SUBST(GST_ALL_LDFLAGS)

dnl this really should only contain flags, not libs - they get added before
dnl whatevertarget_LIBS and -L flags here affect the rest of the linking
GST_PLUGIN_LDFLAGS="-module -avoid-version -export-symbols-regex '^[_]*gst_plugin_desc\$\$' $GST_ALL_LDFLAGS"
AC_SUBST(GST_PLUGIN_LDFLAGS)

dnl *** output files ***

AC_CONFIG_FILES(
Makefile
common/Makefile
common/m4/Makefile
gst-libs/Makefile
gst-libs/gst/Makefile
gst/Makefile
gst/asfdemux/Makefile
gst/dvdlpcmdec/Makefile
gst/dvdsub/Makefile
gst/iec958/Makefile
gst/mpegaudioparse/Makefile
gst/mpegstream/Makefile
gst/realmedia/Makefile
gst/synaesthesia/Makefile
ext/Makefile
ext/a52dec/Makefile
ext/amrnb/Makefile
ext/amrwbdec/Makefile
ext/cdio/Makefile
ext/dvdread/Makefile
ext/lame/Makefile
ext/mad/Makefile
ext/mpeg2dec/Makefile
ext/sidplay/Makefile
ext/twolame/Makefile
ext/x264/Makefile
docs/Makefile
docs/plugins/Makefile
docs/version.entities
tests/Makefile
tests/check/Makefile
m4/Makefile
po/Makefile.in
pkgconfig/Makefile
pkgconfig/gstreamer-plugins-ugly-uninstalled.pc
gst-plugins-ugly.spec
)

dnl Create the config.h file for Visual Studio builds
dnl Beware of spaces and /'s in some of the shell variable contents.
sed \
    -e 's/.*config.h.in.*autoheader.*/\/* Autogenerated config.h created for win32 Visual Studio builds *\/\n\n\/* PREFIX -- specifically added for Windows for easier moving *\/\n#define PREFIX "C:\\\\gstreamer"\n\n/' \
    -e 's/.* GETTEXT_PACKAGE$/#define GETTEXT_PACKAGE "'$GETTEXT_PACKAGE'"/' \
    -e 's/.* GST_DATADIR$/#define GST_DATADIR PREFIX "\\\\share"/' \
    -e 's/.* GST_LEVEL_DEFAULT$/#define GST_LEVEL_DEFAULT GST_LEVEL_ERROR/' \
    -e 's/.* GST_LICENSE$/#define GST_LICENSE "'$GST_LICENSE'"/' \
    -e 's/.* GST_MAJORMINOR$/#define GST_MAJORMINOR "'$GST_MAJORMINOR'"/' \
    -e "s,.* GST_PACKAGE_NAME$,#define GST_PACKAGE_NAME \"${GST_PACKAGE_NAME}\"," \
    -e 's/.* GST_PACKAGE_ORIGIN$/#define GST_PACKAGE_ORIGIN "Unknown package origin"/' \
    -e 's/.* HAVE_CPU_I386$/#define HAVE_CPU_I386 1/' \
    -e 's/.* HAVE_FGETPOS$/#define HAVE_FGETPOS 1/' \
    -e 's/.* HAVE_FSETPOS$/#define HAVE_FSETPOS 1/' \
    -e 's/.* HAVE_LIBXML2$/#define HAVE_LIBXML2 1/' \
    -e 's/.* HAVE_PROCESS_H$/#define HAVE_PROCESS_H 1/' \
    -e 's/.* HAVE_STDLIB_H$/#define HAVE_STDLIB_H 1/' \
    -e 's/.* HAVE_STRING_H$/#define HAVE_STRING_H 1/' \
    -e 's/.* HAVE_SYS_STAT_H$/#define HAVE_SYS_STAT_H 1/' \
    -e 's/.* HAVE_SYS_TYPES_H$/#define HAVE_SYS_TYPES_H 1/' \
    -e 's/.* HAVE_WIN32$/#define HAVE_WIN32 1/' \
    -e 's/.* HAVE_WINSOCK2_H$/#define HAVE_WINSOCK2_H 1/' \
    -e 's/.* HOST_CPU$/#define HOST_CPU "i686"/' \
    -e 's/.* LIBDIR$/#ifdef _DEBUG\n#  define LIBDIR PREFIX "\\\\debug\\\\lib"\n#else\n#  define LIBDIR PREFIX "\\\\lib"\n#endif/' \
    -e 's/.* LOCALEDIR$/#define LOCALEDIR PREFIX "\\\\share\\\\locale"/' \
    -e "s/.* PACKAGE$/#define PACKAGE \"$PACKAGE\"/" \
    -e 's/.* PACKAGE_BUGREPORT$/#define PACKAGE_BUGREPORT "http:\/\/bugzilla.gnome.org\/enter_bug.cgi?product=GStreamer"/' \
    -e "s/.* PACKAGE_NAME$/#define PACKAGE_NAME \"$PACKAGE_NAME\"/" \
    -e "s/.* PACKAGE_STRING$/#define PACKAGE_STRING \"$PACKAGE_STRING\"/" \
    -e 's/.* PACKAGE_TARNAME$/#define PACKAGE_TARNAME "'$PACKAGE_TARNAME'"/' \
    -e 's/.* PACKAGE_VERSION$/#define PACKAGE_VERSION "'$PACKAGE_VERSION'"/' \
    -e 's/.* PLUGINDIR$/#ifdef _DEBUG\n#  define PLUGINDIR PREFIX "\\\\debug\\\\lib\\\\gstreamer-0.10"\n#else\n#  define PLUGINDIR PREFIX "\\\\lib\\\\gstreamer-0.10"\n#endif/' \
    -e 's/.* USE_BINARY_REGISTRY$/#define USE_BINARY_REGISTRY/' \
    -e 's/.* VERSION$/#define VERSION "'$VERSION'"/' \
    -e "s/.* DEFAULT_AUDIOSINK$/#define DEFAULT_AUDIOSINK \"directaudiosink\"/" \
    -e "s/.* DEFAULT_AUDIOSRC$/#define DEFAULT_AUDIOSRC \"audiotestsrc\"/" \
    -e "s/.* DEFAULT_VIDEOSRC$/#define DEFAULT_VIDEOSRC \"videotestsrc\"/" \
    -e "s/.* DEFAULT_VISUALIZER$/#define DEFAULT_VISUALIZER \"goom\"/" \
    config.h.in >win32/common/config.h-new

AC_OUTPUT

AG_GST_OUTPUT_PLUGINS

ORC_OUTPUT
