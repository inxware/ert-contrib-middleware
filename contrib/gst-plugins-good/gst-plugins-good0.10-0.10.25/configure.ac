AC_PREREQ(2.60)

dnl please read gstreamer/docs/random/autotools before changing this file

dnl initialize autoconf
dnl releases only do -Wall, git and prerelease does -Werror too
dnl use a three digit version number for releases, and four for git/pre
AC_INIT(GStreamer Good Plug-ins, 0.10.25,
    http://bugzilla.gnome.org/enter_bug.cgi?product=GStreamer,
    gst-plugins-good)

AG_GST_INIT

dnl initialize automake
AM_INIT_AUTOMAKE([-Wno-portability 1.10])

dnl define PACKAGE_VERSION_* variables
AS_VERSION

dnl check if this is a release version
AS_NANO(GST_GIT="no", GST_GIT="yes")

dnl can autoconf find the source ?
AC_CONFIG_SRCDIR([gst/law/alaw.c])

dnl define the output header for config
AM_CONFIG_HEADER([config.h])

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
dnl AS_LIBTOOL_TAGS

AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

dnl *** required versions of GStreamer stuff ***
GST_REQ=0.10.30
GSTPB_REQ=0.10.30

dnl *** autotools stuff ****

dnl allow for different autotools
AS_AUTOTOOLS_ALTERNATE

dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4 -I common/m4")

dnl set up gettext
dnl the version check needs to stay here because autopoint greps for it
AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])
AG_GST_GETTEXT([gst-plugins-good-$GST_MAJORMINOR])

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

dnl determine c++ preprocessor
dnl FIXME: do we need this ?
AC_PROG_CXXCPP

AS_PROG_OBJC

dnl check if the compiler supports '-c' and '-o' options
AM_PROG_CC_C_O

dnl find an assembler
AM_PROG_AS

dnl check if the compiler supports do while(0) macros
AG_GST_CHECK_DOWHILE_MACROS

AC_PATH_PROG(VALGRIND_PATH, valgrind, no)
AM_CONDITIONAL(HAVE_VALGRIND, test ! "x$VALGRIND_PATH" = "xno")

dnl check for gconftool-2
dnl this macro defines an am conditional, so it needs to be run always
AM_GCONF_SOURCE_2

dnl check for documentation tools
GTK_DOC_CHECK([1.3])
AS_PATH_PYTHON([2.1])
AG_GST_PLUGIN_DOCS([1.3],[2.1])

dnl *** checks for libraries ***

dnl check for libm, for sin()
AC_CHECK_LIBM
AC_SUBST(LIBM)

dnl *** checks for header files ***

dnl check if we have ANSI C header files
AC_HEADER_STDC

dnl used by ext/wavpack
AX_CREATE_STDINT_H

dnl used in gst/rtp/gstasteriskh263.c
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([winsock2.h], HAVE_WINSOCK2_H=yes)
AM_CONDITIONAL(HAVE_WINSOCK2_H, test "x$HAVE_WINSOCK2_H" = "xyes")

dnl used in gst/udp
AC_CHECK_HEADERS([sys/time.h])

dnl *** checks for types/defines ***

dnl Check for FIONREAD ioctl declaration.  This check is needed
dnl for the UDP plugin to build on Solaris
GST_CHECK_FIONREAD

dnl *** checks for structures ***

dnl *** checks for compiler characteristics ***

dnl check if we have GCC inline-asm
AS_GCC_INLINE_ASSEMBLY([HAVE_GCC_ASM=yes], [HAVE_GCC_ASM=no])
if test x$HAVE_GCC_ASM = xyes ; then
  AC_DEFINE(HAVE_GCC_ASM, 1,
    [Define if compiler supports gcc inline assembly])
fi
AM_CONDITIONAL(HAVE_GCC_ASM, test "x$HAVE_GCC_ASM" = "xyes")

dnl *** checks for library functions ***

LIBS_SAVE=$LIBS
LIBS="$LIBS $LIBM"
AC_CHECK_FUNCS(rint sinh cosh asinh fpclass)
LIBS=$LIBS_SAVE

dnl Check whether isinf() is defined by math.h
AC_CACHE_CHECK([for isinf], ac_cv_have_isinf,
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <math.h>]], [[float f = 0.0; int i=isinf(f)]])],[ac_cv_have_isinf="yes"],[ac_cv_have_isinf="no"]))
if test "$ac_cv_have_isinf" = "yes"
then
    AC_DEFINE(HAVE_ISINF, 1, [Define to 1 if you have the `isinf' function.])
fi

dnl Check for mmap (needed by electricfence plugin)
AC_FUNC_MMAP
AM_CONDITIONAL(GST_HAVE_MMAP, test "x$ac_cv_func_mmap_fixed_mapped" = "xyes")

dnl Check for mmap (needed by electricfence plugin)

AC_CHECK_TYPE([struct ip_mreqn], [
              AC_DEFINE(HAVE_IP_MREQN,, [Define if we have struct ip_mreqn])],,
              [#include <netinet/in.h>])

dnl *** checks for dependency libraries ***

dnl GLib is required
AG_GST_GLIB_CHECK([2.18])

dnl Orc
ORC_CHECK([0.4.5])


dnl checks for gstreamer
dnl uninstalled is selected preferentially -- see pkg-config(1)
AG_GST_CHECK_GST($GST_MAJORMINOR, [$GST_REQ], yes)
AG_GST_CHECK_GST_BASE($GST_MAJORMINOR, [$GST_REQ], yes)
AG_GST_CHECK_GST_GDP($GST_MAJORMINOR, [$GST_REQ], yes)
AG_GST_CHECK_GST_CONTROLLER($GST_MAJORMINOR, [$GST_REQ], yes)
AG_GST_CHECK_GST_CHECK($GST_MAJORMINOR, [$GST_REQ], no)
AG_GST_CHECK_GST_PLUGINS_BASE($GST_MAJORMINOR, [$GSTPB_REQ], yes)

GST_TOOLS_DIR=`$PKG_CONFIG --variable=toolsdir gstreamer-$GST_MAJORMINOR`
if test -z $GST_TOOLS_DIR; then
  AC_MSG_ERROR([no tools dir defined in GStreamer pkg-config file; core upgrade needed.])
fi
AC_SUBST(GST_TOOLS_DIR)

AC_MSG_NOTICE(Using GStreamer Core Plugins in $GST_PLUGINS_DIR)
AC_MSG_NOTICE(Using GStreamer Base Plugins in $GSTPB_PLUGINS_DIR)

AM_CONDITIONAL(HAVE_GST_CHECK, test "x$HAVE_GST_CHECK" = "xyes")

dnl Check for documentation xrefs
GLIB_PREFIX="`$PKG_CONFIG --variable=prefix glib-2.0`"
GST_PREFIX="`$PKG_CONFIG --variable=prefix gstreamer-$GST_MAJORMINOR`"
GSTPB_PREFIX="`$PKG_CONFIG --variable=prefix gstreamer-plugins-base-$GST_MAJORMINOR`"
AC_SUBST(GLIB_PREFIX)
AC_SUBST(GST_PREFIX)
AC_SUBST(GSTPB_PREFIX)

dnl GTK is optional and used in examples
if test "x$BUILD_EXAMPLES" = "xyes"; then
  PKG_CHECK_MODULES(GTK, gtk+-3.0, HAVE_GTK=yes,
  [PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.14.0, HAVE_GTK=yes, HAVE_GTK=no)])
  AC_SUBST(GTK_LIBS)
  AC_SUBST(GTK_CFLAGS)
fi
AM_CONDITIONAL(HAVE_GTK, test "x$HAVE_GTK" = "xyes")

dnl should we install schemas ?
translit(dnm, m, l) AM_CONDITIONAL(USE_GCONFTOOL, true)
AG_GST_CHECK_FEATURE(GCONFTOOL, [GConf schemas], , [
  AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
  if test x$GCONFTOOL = xno; then
    AC_MSG_WARN(Not installing GConf schemas)
    HAVE_GCONFTOOL="no"
  else
    HAVE_GCONFTOOL="yes"
  fi
  AC_SUBST(HAVE_GCONFTOOL)
])

dnl *** set variables based on configure arguments ***

dnl set license and copyright notice
GST_LICENSE="LGPL"
AC_DEFINE_UNQUOTED(GST_LICENSE, "$GST_LICENSE", [GStreamer license])
AC_SUBST(GST_LICENSE)

dnl set location of plugin directory
AG_GST_SET_PLUGINDIR

dnl define an ERROR_CFLAGS Makefile variable
dnl -Wundef: too many broken headers
dnl -Waggregate-return: causes problems with ObjC code like in osxvideosink
AG_GST_SET_ERROR_CFLAGS($GST_GIT, [
        -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls
        -Wwrite-strings -Wold-style-definition
        -Winit-self -Wmissing-include-dirs -Waddress -Wno-multichar
        -Wnested-externs ])

dnl define an ERROR_CXXFLAGS Makefile variable
AG_GST_SET_ERROR_CXXFLAGS($GST_GIT, [
        -Wmissing-declarations -Wredundant-decls
        -Wwrite-strings
        -Winit-self -Wmissing-include-dirs -Waddress -Wno-multichar ])

dnl define correct level for debugging messages
AG_GST_SET_LEVEL_DEFAULT($GST_GIT)

dnl used in examples
AG_GST_DEFAULT_ELEMENTS

dnl *** plug-ins to include ***

dnl these are all the gst plug-ins, compilable without additional libs
dnl videofilter is at the top because others depend on it
AG_GST_CHECK_PLUGIN(videofilter)
AG_GST_CHECK_PLUGIN(alpha)
AG_GST_CHECK_PLUGIN(apetag)
AG_GST_CHECK_PLUGIN(audiofx)
AG_GST_CHECK_PLUGIN(auparse)
AG_GST_CHECK_PLUGIN(autodetect)
AG_GST_CHECK_PLUGIN(avi)
AG_GST_CHECK_PLUGIN(cutter)
AG_GST_CHECK_PLUGIN(debugutils)
AG_GST_CHECK_PLUGIN(deinterlace)
AG_GST_CHECK_PLUGIN(effectv)
AG_GST_CHECK_PLUGIN(equalizer)
AG_GST_CHECK_PLUGIN(flv)
AG_GST_CHECK_PLUGIN(id3demux)
AG_GST_CHECK_PLUGIN(icydemux)
AG_GST_CHECK_PLUGIN(interleave)
AG_GST_CHECK_PLUGIN(flx)
AG_GST_CHECK_PLUGIN(goom)
AG_GST_CHECK_PLUGIN(goom2k1)
AG_GST_CHECK_PLUGIN(imagefreeze)
AG_GST_CHECK_PLUGIN(law)
AG_GST_CHECK_PLUGIN(level)
AG_GST_CHECK_PLUGIN(matroska)
AG_GST_CHECK_PLUGIN(monoscope)
AG_GST_CHECK_PLUGIN(multifile)
AG_GST_CHECK_PLUGIN(multipart)
AG_GST_CHECK_PLUGIN(qtdemux)
AG_GST_CHECK_PLUGIN(replaygain)
AG_GST_CHECK_PLUGIN(rtp)
AG_GST_CHECK_PLUGIN(rtpmanager)
AG_GST_CHECK_PLUGIN(rtsp)
AG_GST_CHECK_PLUGIN(shapewipe)
AG_GST_CHECK_PLUGIN(smpte)
AG_GST_CHECK_PLUGIN(spectrum)
AG_GST_CHECK_PLUGIN(udp)
AG_GST_CHECK_PLUGIN(videobox)
AG_GST_CHECK_PLUGIN(videocrop)
AG_GST_CHECK_PLUGIN(videomixer)
AG_GST_CHECK_PLUGIN(wavenc)
AG_GST_CHECK_PLUGIN(wavparse)
AG_GST_CHECK_PLUGIN(y4m)

dnl *** checks for socket and nsl libraries ***
AC_CHECK_FUNC(socket,,[AC_CHECK_LIB(socket,socket)])

dnl disable gst plugins we might not be able to build on this
dnl platform: udp and rtsp (ugly but minimally invasive)
dnl FIXME: maybe move to sys
AC_CHECK_HEADERS([sys/socket.h], HAVE_SYS_SOCKET_H=yes)
AC_CHECK_HEADERS([winsock2.h], HAVE_WINSOCK2_H=yes)

if test "x$HAVE_SYS_SOCKET_H" != "xyes" -a "x$HAVE_WINSOCK2_H" != "xyes"; then
  AG_GST_DISABLE_PLUGIN(udp)
  AG_GST_DISABLE_PLUGIN(rtsp)
fi

if test "x$HAVE_WINSOCK2_H" = "xyes"; then
  WIN32_LIBS="-lws2_32"
  AC_SUBST(WIN32_LIBS)
fi

dnl disable experimental plug-ins
if test "x$BUILD_EXPERIMENTAL" != "xyes"; then
  AG_GST_DISABLE_PLUGIN(monoscope)
fi

dnl *** sys plug-ins ***

echo
AC_MSG_NOTICE([Checking libraries for plugins in sys/])
echo

dnl DirectSound
translit(dnm, m, l) AM_CONDITIONAL(USE_DIRECTSOUND, true)
AG_GST_CHECK_FEATURE(DIRECTSOUND, [DirectSound plug-in], directsoundsink, [
  HAVE_DIRECTSOUND="no"
  save_CFLAGS="$CFLAGS"
  save_LDFLAGS="$LDFLAGS"
  save_LIBS="$LIBS"
  CFLAGS="$CFLAGS $DIRECTSOUND_CFLAGS"
  LDFLAGS="$LDFLAGS $DIRECTSOUND_LDFLAGS"
  LIBS="$LIBS -ldsound -ldxerr9 -luser32"
  AC_MSG_CHECKING(for DirectSound LDFLAGS)
  AC_LINK_IFELSE([
#include <windows.h>
#include <dxerr9.h>
#include <dsound.h>

int main ()
{
  DXGetErrorString9 (0);
  DirectSoundCreate(NULL, NULL, NULL);

  return 0;
}
],
    [HAVE_DIRECTSOUND="yes"],
    [HAVE_DIRECTSOUND="no"])
  AC_MSG_RESULT($HAVE_DIRECTSOUND)
  CFLAGS=$save_CFLAGS
  LDFLAGS=$save_LDFLAGS
  LIBS=$save_LIBS

  if test "x$HAVE_DIRECTSOUND" = "xyes";  then
    dnl this is much more than we want
    DIRECTSOUND_LIBS="-ldsound -ldxerr9 -luser32"
    AC_SUBST(DIRECTSOUND_CFLAGS)
    AC_SUBST(DIRECTSOUND_LDFLAGS)
    AC_SUBST(DIRECTSOUND_LIBS)
  fi
  AC_SUBST(HAVE_DIRECTSOUND)
])

dnl *** OSS audio *** (Linux, *BSD)
translit(dnm, m, l) AM_CONDITIONAL(USE_OSS, true)
AG_GST_CHECK_FEATURE(OSS, [OSS audio], ossaudio, [
 HAVE_OSS="yes"
dnl Linux and newer BSD versions :
 AC_CHECK_HEADER(sys/soundcard.h, [
   AC_DEFINE(HAVE_OSS_INCLUDE_IN_SYS,, [Define if OSS includes are in /sys/])
   ] , [
dnl Some old BSD versions and also newer OpenBSD versions :
   AC_CHECK_HEADER(soundcard.h, [
     AC_DEFINE(HAVE_OSS_INCLUDE_IN_ROOT,, [Define if OSS includes are in /])
     ], [
  dnl Some old BSD versions :
     AC_CHECK_HEADER(machine/soundcard.h, [
       AC_DEFINE(HAVE_OSS_INCLUDE_IN_MACHINE,,
         [Define if OSS includes are in /machine/])
       ], [
       HAVE_OSS="no"
     ])
   ])
 ])
])

dnl *** oss4 ***
translit(dnm, m, l) AM_CONDITIONAL(USE_OSS4, true)
AG_GST_CHECK_FEATURE(OSS4, [Open Sound System 4], oss4, [
  AC_MSG_CHECKING([Checking if we can build the OSS4 elements])
  AC_CHECK_HEADERS([fcntl.h sys/ioctl.h sys/stat.h sys/types.h],
      [test -z "$OSS4_MISSING_HEADER" && HAVE_OSS4="yes"],
      [OSS4_MISSING_HEADER="yes";HAVE_OSS4="no"])
])

dnl *** Sun Audio ***
translit(dnm, m, l) AM_CONDITIONAL(USE_SUNAUDIO, true)
AG_GST_CHECK_FEATURE(SUNAUDIO, [Sun Audio], sunaudio, [
  AC_CHECK_HEADER(sys/audioio.h, HAVE_SUNAUDIO="yes", HAVE_SUNAUDIO="no")
])

dnl *** OSX Audio ***
translit(dnm, m, l) AM_CONDITIONAL(USE_OSX_AUDIO, true)
AG_GST_CHECK_FEATURE(OSX_AUDIO, [OSX audio], osxaudio, [
  AC_CHECK_HEADER(CoreAudio/CoreAudio.h, HAVE_OSX_AUDIO="yes", HAVE_OSX_AUDIO="no")
])

dnl *** OS X video ***
translit(dnm, m, l) AM_CONDITIONAL(USE_OSX_VIDEO, true)
HAVE_OSX_VIDEO="no"
AG_GST_CHECK_FEATURE(OSX_VIDEO, [OSX video], osxvideosink, [
  AC_CHECK_HEADER(OpenGL/gl.h, HAVE_OSX_VIDEO="yes", HAVE_OSX_VIDEO="no")
])
dnl in case header OpenGL/gl.h is found on other platforms
case "$host" in
  *-*darwin*)
    dnl do nothing
    ;;
  *)
    HAVE_OSX_VIDEO="no"
    ;;
esac


dnl *** Video 4 Linux 2 ***
dnl for information about the header/define, see sys/v4l2/gstv4l2element.h
dnl renamed to GST_V4L2 because of some conflict with kernel headers
translit(dnm, m, l) AM_CONDITIONAL(USE_GST_V4L2, true)
AG_GST_CHECK_FEATURE(GST_V4L2, [Video 4 Linux 2], v4l2src, [
  AC_MSG_CHECKING([Checking for up to date v4l2 installation])
  AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef __sun /* Solaris */
#include <sys/types.h>
#include <sys/videodev2.h>
#else /* Linux */
#include <linux/types.h>
#define _LINUX_TIME_H
#define __user
#include <linux/videodev2.h>
#endif
#if defined(V4L2_MAJOR_VERSION) || defined(V4L2_MINOR_VERSION)
#error too early v4l2 version or no v4l2 at all
#endif
    ], [
return 0;
    ], [
      HAVE_GST_V4L2="yes"
      AC_MSG_RESULT(yes)
    ], [
      HAVE_GST_V4L2="no"
      AC_MSG_RESULT(no)

      HAVE_VIDEODEV=no
      AC_CHECK_HEADER(linux/videodev2.h, [ HAVE_VIDEODEV=yes ],
      [
        AC_CHECK_HEADER(sys/videodev2.h, [ HAVE_VIDEODEV=yes ])
      ])

      if test "x$HAVE_VIDEODEV" = "xyes"; then
        AC_MSG_WARN([video4linux2 headers were found, but they're old.])
        AC_MSG_WARN([Please update v4l2 to compile the v4l2 plugins])
      else
        AC_MSG_WARN([video4linux2 was not found])
      fi
    ])

  if [ test x$HAVE_GST_V4L2 = xyes ]; then
    dnl check for missing v4l2_buffer declaration (see #135919)
    MISSING_DECL=0
    AC_MSG_CHECKING(struct v4l2_buffer declaration)
    AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef __sun /* Solaris */
#include <sys/types.h>
#include <sys/videodev2.h>
#else /* Linux */
#include <linux/types.h>
#define _LINUX_TIME_H
#define __user
#include <linux/videodev2.h>
#endif
    ],[
struct v4l2_buffer buf;
buf.index = 0;
return 0;
    ], [ AC_MSG_RESULT(yes) ], [ MISSING_DECL=1 && AC_MSG_RESULT(no) ])
    if [ test x$MISSING_DECL = x1 ]; then
      AC_DEFINE(GST_V4L2_MISSING_BUFDECL, 1, [struct v4l2_buffer missing])
    fi

    dnl check for XOverlay libraries
    AG_GST_CHECK_XV
  fi
])

# Optional gudev for device probing
AC_ARG_WITH([gudev],
  AC_HELP_STRING([--with-gudev],
    [device detection with gudev]),
  [],
  [with_gudev=check])
if test x$HAVE_GST_V4L2 = xyes; then
  if test x$with_gudev != xno; then
      PKG_CHECK_MODULES(GUDEV, [ gudev-1.0 >= 143 ],
          [ have_gudev=yes
            AC_DEFINE(HAVE_GUDEV, 1,
               [Whether gudev is available for device detection])
            AC_DEFINE([G_UDEV_API_IS_SUBJECT_TO_CHANGE], 1, [I know the API is subject to change.])
          ], [
          have_gudev=no
          ])
  else
    have_gudev=no
  fi
fi
AC_SUBST(GUDEV_CFLAGS)
AC_SUBST(GUDEV_LIBS)

# Make libv4l2 non-automagic
AC_ARG_WITH([libv4l2],
  AC_HELP_STRING([--with-libv4l2],
    [support video buffer conversion using libv4l2]),
  [],
  [with_libv4l2=check])
if test x$HAVE_GST_V4L2 = xyes; then
  if test x$with_libv4l2 != xno; then
      PKG_CHECK_MODULES(LIBV4L2, libv4l2,
          [ have_libv4l2=yes
            AC_DEFINE(HAVE_LIBV4L2, 1,
	        [Whether libv4l2 is available for video buffer conversion])
          ], [
          have_libv4l2=no
          ])
  else
    have_libv4l2=no
  fi
fi

dnl Check for X11
translit(dnm, m, l) AM_CONDITIONAL(USE_X, true)
AG_GST_CHECK_FEATURE(X, [X libraries and plugins],
                  [ximagesrc], [
  AC_PATH_XTRA

  dnl now try to find the HEADER
  ac_cflags_save="$CFLAGS"
  ac_cppflags_save="$CPPFLAGS"
  CFLAGS="$CFLAGS $X_CFLAGS"
  CPPFLAGS="$CPPFLAGS $X_CFLAGS"
  AC_CHECK_HEADER(X11/Xlib.h, HAVE_X="yes", HAVE_X="no")

  if test "x$HAVE_X" = "xno"
  then
    AC_MSG_NOTICE([cannot find X11 development files])
  else
    dnl this is much more than we want
    X_LIBS="$X_LIBS $X_PRE_LIBS $X_EXTRA_LIBS"
    dnl AC_PATH_XTRA only defines the path needed to find the X libs,
    dnl it does not add the libs; therefore we add them here
    X_LIBS="$X_LIBS -lX11"
    AC_SUBST(X_CFLAGS)
    AC_SUBST(X_LIBS)

    dnl check for Xfixes
    PKG_CHECK_MODULES(XFIXES, xfixes, HAVE_XFIXES="yes", HAVE_XFIXES="no")
    if test "x$HAVE_XFIXES" = "xyes"
    then
        XFIXES_CFLAGS="-DHAVE_XFIXES $XFIXES_CFLAGS"
    fi
    AC_SUBST(XFIXES_LIBS)
    AC_SUBST(XFIXES_CFLAGS)

    dnl check for Xdamage
    PKG_CHECK_MODULES(XDAMAGE, xdamage, HAVE_XDAMAGE="yes", HAVE_XDAMAGE="no")
    if test "x$HAVE_XDAMAGE" = "xyes"
    then
        XDAMAGE_CFLAGS="-DHAVE_XDAMAGE $XDAMAGE_CFLAGS"
    fi
    AC_SUBST(XDAMAGE_LIBS)
    AC_SUBST(XDAMAGE_CFLAGS)
  fi
  AC_SUBST(HAVE_X)
  CFLAGS="$ac_cflags_save"
  CPPFLAGS="$ac_cppflags_save"
])

dnl FIXME: this should be rolled into the test above, it's just an additional
dnl feature of the ximagesrc plug-in
dnl This is the same as in gst-plugins-base
dnl check for X Shm
translit(dnm, m, l) AM_CONDITIONAL(USE_XSHM, true)
AG_GST_CHECK_FEATURE(XSHM, [X Shared Memory extension], , [
  if test x$HAVE_X = xyes; then
    AC_CHECK_LIB(Xext, XShmAttach,
                 HAVE_XSHM="yes", HAVE_XSHM="no",
                 $X_LIBS)
    if test "x$HAVE_XSHM" = "xyes"; then
      XSHM_LIBS="-lXext"
    else
      dnl On AIX, it is in XextSam instead, but we still need -lXext
      AC_CHECK_LIB(XextSam, XShmAttach,
                   HAVE_XSHM="yes", HAVE_XSHM="no",
                   $X_LIBS)
      if test "x$HAVE_XSHM" = "xyes"; then
        XSHM_LIBS="-lXext -lXextSam"
      fi
    fi
  fi
], ,[
  AC_SUBST(HAVE_XSHM)
  AC_SUBST(XSHM_LIBS)
])

dnl for V4L2, we also need to know if we have XVIDEO
translit(dnm, m, l) AM_CONDITIONAL(USE_XVIDEO, true)
AG_GST_CHECK_FEATURE(XVIDEO, [X11 XVideo extensions],
                  [], [
  AG_GST_CHECK_XV
])

dnl *** ext plug-ins ***
dnl keep this list sorted alphabetically !

if test "x$BUILD_EXTERNAL" = "xyes"; then

echo
AC_MSG_NOTICE([Checking libraries for plugins in ext/])
echo

dnl *** aalib ***
translit(dnm, m, l) AM_CONDITIONAL(USE_AALIB, true)
AG_GST_CHECK_FEATURE(AALIB, [aalib ASCII Art library], aasink, [
  AM_PATH_AALIB(, HAVE_AALIB=yes, HAVE_AALIB=no)
  AS_SCRUB_INCLUDE(AALIB_CFLAGS)
])

dnl *** annodex ***
translit(dnm, m, l) AM_CONDITIONAL(USE_ANNODEX, true)
AG_GST_CHECK_FEATURE(ANNODEX, [XML library], annodex, [
  AG_GST_PKG_CHECK_MODULES(ANNODEX, libxml-2.0 >= 2.4.9)
])

dnl *** cairo ***
translit(dnm, m, l) AM_CONDITIONAL(USE_CAIRO, true)
AG_GST_CHECK_FEATURE(CAIRO, [Cairo graphics rendering], cairo, [
  AG_GST_PKG_CHECK_MODULES(CAIRO, cairo >= 1.0.0)
])

dnl **** ESound ****
translit(dnm, m, l) AM_CONDITIONAL(USE_ESD, true)
AG_GST_CHECK_FEATURE(ESD, [ESounD sound daemon], esdsink, [
  AG_GST_PKG_CHECK_MODULES(ESD, esound >= 0.2.12)
  if test $HAVE_ESD = no
  then
    AM_PATH_ESD(0.2.12, HAVE_ESD="yes")
    AS_SCRUB_INCLUDE(ESD_CFLAGS)
  fi
])

dnl *** FLAC ***
translit(dnm, m, l) AM_CONDITIONAL(USE_FLAC, true)
AG_GST_CHECK_FEATURE(FLAC, [FLAC lossless audio], flac, [
  AG_GST_PKG_CHECK_MODULES(FLAC, flac >= 1.1.4)
])

dnl *** GConf ***
translit(dnm, m, l) AM_CONDITIONAL(USE_GCONF, true)
AG_GST_CHECK_FEATURE(GCONF, [GConf libraries], gconfelements, [
  AG_GST_PKG_CHECK_MODULES(GCONF, gconf-2.0)
])

dnl *** GDK pixbuf ***
translit(dnm, m, l) AM_CONDITIONAL(USE_GDK_PIXBUF, true)
AG_GST_CHECK_FEATURE(GDK_PIXBUF, [GDK pixbuf], gdkpixbuf, [
  AG_GST_PKG_CHECK_MODULES(GDK_PIXBUF, gdk-pixbuf-2.0 >= 2.8.0)
])

dnl *** HAL ***
translit(dnm, m, l) AM_CONDITIONAL(USE_HAL, true)
AG_GST_CHECK_FEATURE(HAL, [HAL libraries], halelements, [
  AG_GST_PKG_CHECK_MODULES(HAL, [hal >= 0.5.6, dbus-1 >= 0.32])
])

dnl *** jpeg ***
dnl FIXME: we could use header checks here as well IMO
translit(dnm, m, l) AM_CONDITIONAL(USE_JPEG, true)
AG_GST_CHECK_FEATURE(JPEG, [jpeg library], jpeg, [
  AC_ARG_WITH(jpeg-mmx,
    [  --with-jpeg-mmx, path to MMX'ified JPEG library])
  OLD_LIBS="$LIBS"
  if test x$with_jpeg_mmx != x; then
    LIBS="$LIBS -L$with_jpeg_mmx"
  fi
  AC_CHECK_LIB(jpeg-mmx, jpeg_set_defaults, HAVE_JPEG="yes", HAVE_JPEG="no")
  JPEG_LIBS="$LIBS -ljpeg-mmx"
  LIBS="$OLD_LIBS"
  if test x$HAVE_JPEG != xyes; then
    AC_CHECK_LIB(jpeg, jpeg_set_defaults, HAVE_JPEG="yes", HAVE_JPEG="no")
    JPEG_LIBS="-ljpeg"
  fi
  AC_SUBST(JPEG_LIBS)
])

dnl *** libcaca ***
translit(dnm, m, l) AM_CONDITIONAL(USE_LIBCACA, true)
AG_GST_CHECK_FEATURE(LIBCACA, [libcaca coloured ASCII art], cacasink, [
  AG_GST_PKG_CHECK_MODULES(LIBCACA, caca)
  dnl only newer versions of libcaca ship caca.pc, so try caca-config as well
  if test "x$HAVE_LIBCACA" != "xyes"; then
    AG_GST_CHECK_CONFIGPROG(LIBCACA, caca-config)
    dnl see if it compilation works too, might not if we are cross-compiling
    if test "x$HAVE_LIBCACA" = "xyes"; then
      AC_CHECK_LIB([caca], [caca_init], [HAVE_LIBCACA=yes],
                   [HAVE_LIBCACA=no], [$LIBCACA_LIBS])
    fi
  fi
])

dnl *** libdv ***
translit(dnm, m, l) AM_CONDITIONAL(USE_LIBDV, true)
AG_GST_CHECK_FEATURE(LIBDV, [libdv DV demuxer/decoder], dv, [
  AG_GST_PKG_CHECK_MODULES(LIBDV, libdv >= 0.100)
])

dnl *** libpng ***
translit(dnm, m, l) AM_CONDITIONAL(USE_LIBPNG, true)
AG_GST_CHECK_FEATURE(LIBPNG, [Portable Network Graphics library], png, [
  AG_GST_PKG_CHECK_MODULES(LIBPNG, libpng >= 1.2)
])

dnl *** pulseaudio ***
translit(dnm, m, l) AM_CONDITIONAL(USE_PULSE, true)
AG_GST_CHECK_FEATURE(PULSE, [pulseaudio plug-in], pulseaudio, [
  AG_GST_PKG_CHECK_MODULES(PULSE, libpulse >= 0.9.10)
  AG_GST_PKG_CHECK_MODULES(PULSE_0_9_11, libpulse >= 0.9.11)
  if test x$HAVE_PULSE_0_9_11 = xyes; then
    AC_DEFINE(HAVE_PULSE_0_9_11, 1, [defined if pulseaudio >= 0.9.11 is available])
  fi
  AG_GST_PKG_CHECK_MODULES(PULSE_0_9_12, libpulse >= 0.9.12)
  if test x$HAVE_PULSE_0_9_12 = xyes; then
    AC_DEFINE(HAVE_PULSE_0_9_12, 1, [defined if pulseaudio >= 0.9.12 is available])
  fi
  AG_GST_PKG_CHECK_MODULES(PULSE_0_9_13, libpulse >= 0.9.13)
  if test x$HAVE_PULSE_0_9_13 = xyes; then
    AC_DEFINE(HAVE_PULSE_0_9_13, 1, [defined if pulseaudio >= 0.9.13 is available])
  fi
  AG_GST_PKG_CHECK_MODULES(PULSE_0_9_15, libpulse >= 0.9.15)
  if test x$HAVE_PULSE_0_9_15 = xyes; then
    AC_DEFINE(HAVE_PULSE_0_9_15, 1, [defined if pulseaudio >= 0.9.15 is available])
  fi
  AG_GST_PKG_CHECK_MODULES(PULSE_0_9_16, libpulse >= 0.9.16)
  if test x$HAVE_PULSE_0_9_16 = xyes; then
    AC_DEFINE(HAVE_PULSE_0_9_16, 1, [defined if pulseaudio >= 0.9.16 is available])
  fi
  AG_GST_PKG_CHECK_MODULES(PULSE_0_9_20, libpulse >= 0.9.20)
  if test x$HAVE_PULSE_0_9_20 = xyes; then
    AC_DEFINE(HAVE_PULSE_0_9_20, 1, [defined if pulseaudio >= 0.9.20 is available])
  fi
])

dnl *** dv1394 ***
translit(dnm, m, l) AM_CONDITIONAL(USE_DV1394, true)
AG_GST_CHECK_FEATURE(DV1394, [raw1394 and avc1394 library], 1394, [
  dnl first test for libraw1394 >= 2.0.0
  AG_GST_PKG_CHECK_MODULES(RAW1394, libraw1394 >= 2.0.0)
  AG_GST_PKG_CHECK_MODULES(LIBIEC61883, libiec61883 >= 1.0.0)
  dnl now see how far we got
  if test x$HAVE_RAW1394 = xyes && \
     test x$HAVE_LIBIEC61883 = xyes; then
       AG_GST_CHECK_LIBHEADER(AVC1394,
        avc1394, avc1394_send_command, $RAW1394_LIBS,
	libavc1394/avc1394.h, AVC1394_LIBS="-lavc1394")
       AG_GST_CHECK_LIBHEADER(ROM1394,
        rom1394, rom1394_free_directory, $RAW1394_LIBS,
        libavc1394/rom1394.h, ROM1394_LIBS="-lrom1394")
       if test x$HAVE_AVC1394 = xyes && \
          test x$HAVE_AVC1394 = xyes && \
	  test x$HAVE_ROM1394 = xyes; then
	  HAVE_DV1394=yes
          DV1394_LIBS="$RAW1394_LIBS $AVC1394_LIBS $ROM1394_LIBS $LIBIEC61883_LIBS"
          DV1394_CFLAGS="$RAW1394_CFLAGS $LIBIEC61883_CFLAGS -DHAVE_LIBIEC61883"
          AC_SUBST(DV1394_CFLAGS)
          AC_SUBST(DV1394_LIBS)
	fi
  else
       if test x$HAVE_RAW1394 = xno; then
	  AG_GST_PKG_CHECK_MODULES(RAW1394, libraw1394 >= 1.1.0)
	  AG_GST_CHECK_LIBHEADER(AVC1394,
	   avc1394, avc1394_send_command, $RAW1394_LIBS,
	   libavc1394/avc1394.h, AVC1394_LIBS="-lavc1394")
	  AG_GST_CHECK_LIBHEADER(ROM1394,
	   rom1394, rom1394_free_directory, $RAW1394_LIBS,
	   libavc1394/rom1394.h, ROM1394_LIBS="-lrom1394")
          if test x$HAVE_RAW1394 = xyes && \
	     test x$HAVE_AVC1394 = xyes && \
	     test x$HAVE_ROM1394 = xyes; then
	     HAVE_DV1394=yes
	     DV1394_LIBS="$RAW1394_LIBS $AVC1394_LIBS $ROM1394_LIBS"
	     DV1394_CFLAGS="$RAW1394_CFLAGS"
	     if test x$HAVE_LIBIEC61883 = xyes; then
	       DV1394_CFLAGS="$RAW1394_CFLAGS $LIBIEC61883_CFLAGS -DHAVE_LIBIEC61883"
	       DV1394_LIBS="$DV1394_LIBS $LIBIEC61883_LIBS"
	     fi
             AC_SUBST(DV1394_CFLAGS)
             AC_SUBST(DV1394_LIBS)
           fi
	 fi
  fi
])

dnl *** shout2 ***
translit(dnm, m, l) AM_CONDITIONAL(USE_SHOUT2, true)
AG_GST_CHECK_FEATURE(SHOUT2, [Shoutcast/Icecast client library], shout2, [
  AG_GST_PKG_CHECK_MODULES(SHOUT2, shout >= 2.0)
  if test $HAVE_SHOUT2 = no
  then
    AM_PATH_SHOUT2(HAVE_SHOUT2="yes")
    AC_SUBST(SHOUT2_CFLAGS)
    AC_SUBST(SHOUT2_LIBS)
  fi
])

dnl *** soup ***
translit(dnm, m, l) AM_CONDITIONAL(USE_SOUP, true)
AG_GST_CHECK_FEATURE(SOUP, [soup http client plugin (2.4)], souphttpsrc, [
  PKG_CHECK_MODULES(SOUP, libsoup-gnome-2.4 >= 2.3.2, [
    HAVE_SOUP="yes"
    AC_DEFINE(HAVE_LIBSOUP_GNOME, 1, [soup gnome integration])
  ],[
    PKG_CHECK_MODULES(SOUP, libsoup-2.4 >= 2.26, HAVE_SOUP="yes", [
      HAVE_SOUP="no"
      AC_MSG_RESULT(no)
    ])
  ])
  AC_SUBST(SOUP_CFLAGS)
  AC_SUBST(SOUP_LIBS)
])

dnl *** speex >= 1.0.4 or >= 1.1.5 ***
dnl   1.1.4 and earlier were not API/ABI compatible with 1.0
dnl   1.1.6 is the first to use a .pc/pkg-config file ***
dnl   speex_jitter.h is 1.1.x only
translit(dnm, m, l) AM_CONDITIONAL(USE_SPEEX, true)
AG_GST_CHECK_FEATURE(SPEEX, [speex speech codec], speex, [
  AG_GST_PKG_CHECK_MODULES(SPEEX, speex >= 1.1.6)
  if test $HAVE_SPEEX = no
  then
    AG_GST_CHECK_LIBHEADER(SPEEX, speex, speex_bits_init, , speex/speex.h, [
      AC_CHECK_HEADER(speex/speex_jitter.h, [
        dnl speex 1.1.x :
        AG_GST_CHECK_LIBHEADER(SPEEX, speex, speex_encode_int, , speex/speex.h, [
          dnl speex 1.1.5 or + :
          HAVE_SPEEX="yes"
          SPEEX_LIBS="-lspeex"
          AC_SUBST(SPEEX_CFLAGS)
          AC_SUBST(SPEEX_LIBS)
        ],[
          HAVE_SPEEX="no"
        ])
      ],[
        dnl speex 1.0.x :
          AC_CHECK_DECL(SPEEX_GET_LOOKAHEAD, [
            dnl speex 1.0.4
            HAVE_SPEEX="yes"
            SPEEX_LIBS="-lspeex"
            AC_SUBST(SPEEX_CFLAGS)
            AC_SUBST(SPEEX_LIBS)

            AC_DEFINE_UNQUOTED(SPEEX_1_0, 1,
              [defined if speex 1.0.x API detected])
        ],[
            HAVE_SPEEX="no"
            AC_MSG_NOTICE(You need at least 1.0.4 to compile the speex plugin)
        ], [
#include <speex/speex.h>
        ])
      ])
    ])
  fi
])

dnl *** taglib ***
translit(dnm, m, l) AM_CONDITIONAL(USE_TAGLIB, true)
AG_GST_CHECK_FEATURE(TAGLIB, [taglib tagging library], taglib, [
  AG_GST_PKG_CHECK_MODULES(TAGLIB, taglib >= 1.5)
  if test "x$HAVE_CXX" != "xyes"; then
    USE_TAGLIB=false
    AC_MSG_NOTICE([Not building taglib plugin: no C++ compiler found])
  else
    dnl work around bug in taglib 1.5 headers, remove once there is a 1.6
    AS_CXX_COMPILER_FLAG([-Wno-attributes], [
        TAGLIB_CXXFLAGS="$TAGLIB_CFLAGS -Wno-attributes"
      ], [
        TAGLIB_CXXFLAGS="$TAGLIB_CFLAGS"
    ])
    AC_SUBST(TAGLIB_CXXFLAGS)
  fi
])

dnl *** wavpack ***
translit(dnm, m, l) AM_CONDITIONAL(USE_WAVPACK, true)
AG_GST_CHECK_FEATURE(WAVPACK, [wavpack plug-in], wavpack, [
  PKG_CHECK_MODULES(WAVPACK, wavpack >= 4.40.0, HAVE_WAVPACK=yes, [
    PKG_CHECK_MODULES(WAVPACK, wavpack >= 4.20, [
      HAVE_WAVPACK=yes
      AC_DEFINE(WAVPACK_OLD_API, 1, [old wavpack API])
    ],[
      HAVE_WAVPACK=no
      AC_MSG_RESULT(no)
    ])
  ])
  AC_SUBST(WAVPACK_CFLAGS)
  AC_SUBST(WAVPACK_LIBS)
])

dnl *** qtdemux & id3demux & matroska prefer to have zlib ***
translit(dnm, m, l) AM_CONDITIONAL(USE_ZLIB, true)
AG_GST_CHECK_FEATURE(ZLIB, [zlib support for id3demux/qtdemux/matroska],, [
  AG_GST_CHECK_LIBHEADER(ZLIB,
    z, uncompress,, zlib.h, [
    HAVE_ZLIB="yes"
    ZLIB_LIBS="-lz"
    AC_SUBST(ZLIB_LIBS)
  ])
])

dnl *** matroska prefers to have bz2 ***
translit(dnm, m, l) AM_CONDITIONAL(USE_BZ2, true)
AG_GST_CHECK_FEATURE(BZ2, [bz2 library for matroska ],, [
  AG_GST_CHECK_LIBHEADER(BZ2, bz2, BZ2_bzCompress, ,bzlib.h, BZ2_LIBS="-lbz2")
  AC_SUBST(BZ2_LIBS)
])

else

dnl not building plugins with external dependencies,
dnl but we still need to set the conditionals
AM_CONDITIONAL(USE_AALIB, false)
AM_CONDITIONAL(USE_ANNODEX, false)
AM_CONDITIONAL(USE_BZ2, false)
AM_CONDITIONAL(USE_CAIRO, false)
AM_CONDITIONAL(USE_DIRECTSOUND, false)
AM_CONDITIONAL(USE_DV1394, false)
AM_CONDITIONAL(USE_ESD, false)
AM_CONDITIONAL(USE_FLAC, false)
AM_CONDITIONAL(USE_GCONF, false)
AM_CONDITIONAL(USE_GCONFTOOL, false)
AM_CONDITIONAL(USE_GDK_PIXBUF, false)
AM_CONDITIONAL(USE_GST_V4L2, false)
AM_CONDITIONAL(USE_HAL, false)
AM_CONDITIONAL(USE_JPEG, false)
AM_CONDITIONAL(USE_LIBCACA, false)
AM_CONDITIONAL(USE_LIBDV, false)
AM_CONDITIONAL(USE_LIBPNG, false)
AM_CONDITIONAL(USE_OSS, false)
AM_CONDITIONAL(USE_OSS4, false)
AM_CONDITIONAL(USE_OSX_AUDIO, false)
AM_CONDITIONAL(USE_OSX_VIDEO, false)
AM_CONDITIONAL(USE_PULSE, false)
AM_CONDITIONAL(USE_SHOUT2, false)
AM_CONDITIONAL(USE_SOUP, false)
AM_CONDITIONAL(USE_SPEEX, false)
AM_CONDITIONAL(USE_SUNAUDIO, false)
AM_CONDITIONAL(USE_TAGLIB, false)
AM_CONDITIONAL(USE_WAVPACK, false)
AM_CONDITIONAL(USE_X, false)
AM_CONDITIONAL(USE_XSHM, false)
AM_CONDITIONAL(USE_XVIDEO, false)
AM_CONDITIONAL(USE_ZLIB, false)

fi dnl of EXT plugins

dnl *** finalize CFLAGS, LDFLAGS, LIBS

dnl Overview:
dnl GST_OPTION_CFLAGS:  common flags for profiling, debugging, errors, ...
dnl GST_*:              flags shared by all built objects
dnl GST_ALL_LDFLAGS:    linker flags shared by all
dnl GST_LIB_LDFLAGS:    not needed, we don't install libraries
dnl GST_LT_LDFLAGS:     library versioning of our libraries
dnl GST_PLUGIN_LDFLAGS: flags to be used for all plugins

dnl GST_OPTION_CFLAGS
if test "x$USE_DEBUG" = xyes; then
   PROFILE_CFLAGS="-g"
fi
AC_SUBST(PROFILE_CFLAGS)

if test "x$PACKAGE_VERSION_NANO" = "x1"; then
  dnl Define _only_ for git (not pre-releases or releases)
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

dnl our libraries need to be versioned correctly
AC_SUBST(GST_LT_LDFLAGS)

dnl FIXME: do we want to rename to GST_ALL_* ?
dnl prefer internal headers to already installed ones
dnl also add builddir include for enumtypes and marshal
dnl add ERROR_CFLAGS, but overridable
GST_CXXFLAGS="-I\$(top_srcdir)/gst-libs $GST_CFLAGS $GLIB_EXTRA_CFLAGS \$(GST_OPTION_CXXFLAGS)"
GST_CFLAGS="-I\$(top_srcdir)/gst-libs $GST_CFLAGS $GLIB_EXTRA_CFLAGS \$(GST_OPTION_CFLAGS)"
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

dnl keep this alphabetic per directory, please
AC_CONFIG_FILES(
Makefile
gst/Makefile
gst/alpha/Makefile
gst/apetag/Makefile
gst/audiofx/Makefile
gst/auparse/Makefile
gst/autodetect/Makefile
gst/avi/Makefile
gst/cutter/Makefile
gst/deinterlace/Makefile
gst/debugutils/Makefile
gst/effectv/Makefile
gst/equalizer/Makefile
gst/flv/Makefile
gst/id3demux/Makefile
gst/icydemux/Makefile
gst/imagefreeze/Makefile
gst/interleave/Makefile
gst/goom/Makefile
gst/goom2k1/Makefile
gst/law/Makefile
gst/level/Makefile
gst/matroska/Makefile
gst/monoscope/Makefile
gst/multifile/Makefile
gst/multipart/Makefile
gst/qtdemux/Makefile
gst/replaygain/Makefile
gst/rtp/Makefile
gst/rtpmanager/Makefile
gst/rtsp/Makefile
gst/shapewipe/Makefile
gst/smpte/Makefile
gst/spectrum/Makefile
gst/udp/Makefile
gst/videobox/Makefile
gst/videocrop/Makefile
gst/videofilter/Makefile
gst/videomixer/Makefile
gst/wavenc/Makefile
gst/wavparse/Makefile
gst/flx/Makefile
gst/y4m/Makefile
ext/jpeg/Makefile
ext/Makefile
ext/aalib/Makefile
ext/annodex/Makefile
ext/cairo/Makefile
ext/dv/Makefile
ext/esd/Makefile
ext/flac/Makefile
ext/gconf/Makefile
ext/gdk_pixbuf/Makefile
ext/hal/Makefile
ext/libcaca/Makefile
ext/libpng/Makefile
ext/pulse/Makefile
ext/raw1394/Makefile
ext/shout2/Makefile
ext/soup/Makefile
ext/speex/Makefile
ext/taglib/Makefile
ext/wavpack/Makefile
sys/Makefile
sys/directsound/Makefile
sys/oss/Makefile
sys/oss4/Makefile
sys/osxaudio/Makefile
sys/osxvideo/Makefile
sys/sunaudio/Makefile
sys/v4l2/Makefile
sys/waveform/Makefile
sys/ximage/Makefile
farsight/Makefile
farsight/valve/Makefile
farsight/liveadder/Makefile
farsight/dtmf/Makefile
farsight/rtpmux/Makefile
farsight/autoconvert/Makefile
po/Makefile.in
tests/Makefile
tests/check/Makefile
tests/examples/Makefile
tests/examples/audiofx/Makefile
tests/examples/equalizer/Makefile
tests/examples/level/Makefile
tests/examples/rtp/Makefile
tests/examples/shapewipe/Makefile
tests/examples/spectrum/Makefile
tests/examples/v4l2/Makefile
tests/files/Makefile
tests/icles/Makefile
gconf/Makefile
gconf/gstreamer.schemas
common/Makefile
common/m4/Makefile
m4/Makefile
docs/Makefile
docs/plugins/Makefile
docs/version.entities
pkgconfig/Makefile
pkgconfig/gstreamer-plugins-good-uninstalled.pc
gst-plugins-good.spec
)

dnl Create the config.h file for Visual Studio builds
dnl Beware of spaces and /'s in some of the shell variable contents.
sed \
    -e 's/.*config.h.in.*autoheader.*/\/* Autogenerated config.h created for win32 Visual Studio builds *\/\n\n\/* PREFIX -- specifically added for Windows for easier moving *\/\n#define PREFIX "C:\\\\gstreamer"\n\n#define GST_INSTALL_PLUGINS_HELPER PREFIX "\\\\libexec\\\\gst-install-plugins-helper.exe"/' \
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

