dnl Process this file with autoconf to produce a configure script.

dnl First, define all of the version numbers up front.
dnl In particular, this allows the version macro to be used in AC_INIT

dnl These first two version numbers are updated automatically on each release.
m4_define([LIBARCHIVE_VERSION_S],[2.7.0])
m4_define([LIBARCHIVE_VERSION_N],[2007000])

dnl bsdtar and bsdcpio versioning tracks libarchive
m4_define([BSDTAR_VERSION_S],LIBARCHIVE_VERSION_S())
m4_define([BSDCPIO_VERSION_S],LIBARCHIVE_VERSION_S())

#
# Now starts the "real" configure script.
#

AC_INIT([libarchive],LIBARCHIVE_VERSION_S(),[kientzle@freebsd.org])
# Make sure the srcdir contains "libarchive" directory
AC_CONFIG_SRCDIR([libarchive])
# Use auxiliary subscripts from this subdirectory (cleans up root)
AC_CONFIG_AUX_DIR([build/autoconf])
# M4 scripts
AC_CONFIG_MACRO_DIR([build/autoconf])
# Must follow AC_CONFIG macros above...
AM_INIT_AUTOMAKE()

# Libtool versioning uses different conventions on different
# platforms.  At least on FreeBSD, libtool uses an overly complex
# convention that attempts to solve problems that most people just
# don't have and which just causes confusion for most end users.
ARCHIVE_MAJOR=$(( LIBARCHIVE_VERSION_N() / 1000000 ))
ARCHIVE_MINOR=$(( (LIBARCHIVE_VERSION_N() / 1000) % 1000 ))
ARCHIVE_REVISION=$(( LIBARCHIVE_VERSION_N() % 1000 ))
ARCHIVE_LIBTOOL_MAJOR=`echo $((${ARCHIVE_MAJOR} + ${ARCHIVE_MINOR}))`
ARCHIVE_LIBTOOL_VERSION=$ARCHIVE_LIBTOOL_MAJOR:$ARCHIVE_REVISION:$ARCHIVE_MINOR

# Stick the version numbers into config.h
AC_DEFINE([LIBARCHIVE_VERSION_STRING],"LIBARCHIVE_VERSION_S()",
	[Version number of libarchive])
AC_DEFINE_UNQUOTED([LIBARCHIVE_VERSION_NUMBER],"LIBARCHIVE_VERSION_N()",
	[Version number of libarchive as a single integer])
AC_DEFINE([BSDCPIO_VERSION_STRING],"BSDCPIO_VERSION_S()",
	[Version number of bsdcpio])
AC_DEFINE([BSDTAR_VERSION_STRING],"BSDTAR_VERSION_S()",
	[Version number of bsdtar])

# The shell variables here must be the same as the AC_SUBST() variables
# below, but the shell variable names apparently cannot be the same as
# the m4 macro names above.  Why?  Ask autoconf.
BSDCPIO_VERSION_STRING=BSDCPIO_VERSION_S()
BSDTAR_VERSION_STRING=BSDTAR_VERSION_S()
LIBARCHIVE_VERSION_STRING=LIBARCHIVE_VERSION_S()
LIBARCHIVE_VERSION_NUMBER=LIBARCHIVE_VERSION_N()

# Substitute the above version numbers into the various files below.
# Yes, I believe this is the fourth time we define what are essentially
# the same symbols.  Why? Ask autoconf.
AC_SUBST(ARCHIVE_LIBTOOL_VERSION)
AC_SUBST(BSDCPIO_VERSION_STRING)
AC_SUBST(BSDTAR_VERSION_STRING)
AC_SUBST(LIBARCHIVE_VERSION_STRING)
AC_SUBST(LIBARCHIVE_VERSION_NUMBER)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

# Check for host type
AC_CANONICAL_HOST

dnl Compilation on mingw and Cygwin needs special Makefile rules
inc_windows_files=no
inc_cygwin_files=no
case "$host_os" in
  *mingw* ) inc_windows_files=yes ;;
  *cygwin*) inc_cygwin_files=yes ;;
esac
AM_CONDITIONAL([INC_WINDOWS_FILES], [test $inc_windows_files = yes])
AM_CONDITIONAL([INC_CYGWIN_FILES], [test $inc_cygwin_files = yes])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_YACC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_CHECK_TOOL([STRIP],[strip])

#
# Locate some specific link objects for cygwin
#
prog_ldadd_extra=
case "$host_os" in
*cygwin*)
  binmode_path=`$CC -print-file-name=binmode.o`
  if test -n "$binmode_path" && test -f "$binmode_path" ; then
    prog_ldadd_extra="$binmode_path"
  fi
  ;;
esac
PROG_LDADD_EXTRA=$prog_ldadd_extra
AC_SUBST(PROG_LDADD_EXTRA)

#
# Options for building bsdtar.
#
# Default is to build bsdtar, but allow people to override that.
#
AC_ARG_ENABLE([bsdtar],
	[AS_HELP_STRING([--enable-bsdtar], [enable build of bsdtar (default)])
	AS_HELP_STRING([--enable-bsdtar=static], [force static build of bsdtar])
	AS_HELP_STRING([--enable-bsdtar=shared], [force dynamic build of bsdtar])
AS_HELP_STRING([--disable-bsdtar], [disable build of bsdtar])],
	[], [enable_bsdtar=yes])

case "$enable_bsdtar" in
yes)
	if test "$enable_static" = "no"; then
		static_bsdtar=no
	else
		static_bsdtar=yes
	fi
	build_bsdtar=yes
	;;
dynamic|shared)
	if test "$enable_shared" = "no"; then
		AC_MSG_FAILURE([Shared linking of bsdtar requires shared libarchive])
	fi
	build_bsdtar=yes
	static_bsdtar=no
	;;
static)
	build_bsdtar=yes
	static_bsdtar=yes
	;;
no)
	build_bsdtar=no
	static_bsdtar=no
	;;
*)
	AC_MSG_FAILURE([Unsupported value for --enable-bsdtar])
	;;
esac

AM_CONDITIONAL([BUILD_BSDTAR], [ test "$build_bsdtar" = yes ])
AM_CONDITIONAL([STATIC_BSDTAR], [ test "$static_bsdtar" = yes ])

#
# Options for building bsdcpio.
#
# Default is not to build bsdcpio, but that can be overridden.
#
AC_ARG_ENABLE([bsdcpio],
	[AS_HELP_STRING([--enable-bsdcpio], [enable build of bsdcpio (default)])
	AS_HELP_STRING([--enable-bsdcpio=static], [static build of bsdcpio])
	AS_HELP_STRING([--enable-bsdcpio=shared], [dynamic build of bsdcpio])
AS_HELP_STRING([--disable-bsdcpio], [disable build of bsdcpio])],
	[], [enable_bsdcpio=yes])

case "$enable_bsdcpio" in
yes)
	if test "$enable_static" = "no"; then
	   static_bsdcpio=no
	else
	   static_bsdcpio=yes
        fi
	build_bsdcpio=yes
	;;
dynamic|shared)
	if test "$enabled_shared" = "no"; then
	   AC_MSG_FAILURE([Shared linking of bsdcpio requires shared libarchive])
	fi
	build_bsdcpio=yes
	;;
static)
	build_bsdcpio=yes
	static_bsdcpio=yes
	;;
no)
	build_bsdcpio=no
	static_bsdcpio=no
	;;
*)
	AC_MSG_FAILURE([Unsupported value for --enable-bsdcpio])
	;;
esac

AM_CONDITIONAL([BUILD_BSDCPIO], [ test "$build_bsdcpio" = yes ])
AM_CONDITIONAL([STATIC_BSDCPIO], [ test "$static_bsdcpio" = yes ])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([acl/libacl.h attr/xattr.h ctype.h errno.h])
AC_CHECK_HEADERS([ext2fs/ext2_fs.h fcntl.h grp.h])
AC_CHECK_HEADERS([inttypes.h langinfo.h limits.h linux/fs.h])
AC_CHECK_HEADERS([locale.h paths.h poll.h pwd.h regex.h signal.h stdarg.h])
AC_CHECK_HEADERS([stdint.h stdlib.h string.h])
AC_CHECK_HEADERS([sys/acl.h sys/extattr.h sys/ioctl.h sys/mkdev.h])
AC_CHECK_HEADERS([sys/param.h sys/poll.h sys/select.h sys/time.h sys/utime.h])
AC_CHECK_HEADERS([time.h unistd.h utime.h wchar.h wctype.h windows.h])

# Checks for libraries.
AC_ARG_WITH([zlib],
  AS_HELP_STRING([--without-zlib], [Don't build support for gzip through zlib]))

if test "x$with_zlib" != "xno"; then
  AC_CHECK_HEADERS([zlib.h])
  AC_CHECK_LIB(z,inflate)
fi

AC_ARG_WITH([bz2lib],
  AS_HELP_STRING([--without-bz2lib], [Don't build support for bzip2 through bz2lib]))

if test "x$with_bz2lib" != "xno"; then
  AC_CHECK_HEADERS([bzlib.h])
  AC_CHECK_LIB(bz2,BZ2_bzDecompressInit)
fi

AC_ARG_WITH([lzmadec],
  AS_HELP_STRING([--without-lzmadec], [Don't build support for lzma through lzmadec]))

if test "x$with_lzmadec" != "xno"; then
  AC_CHECK_HEADERS([lzmadec.h])
  AC_CHECK_LIB(lzmadec,lzmadec_decode)
fi

AC_ARG_WITH([lzma],
  AS_HELP_STRING([--without-lzma], [Don't build support for xz through lzma]))

if test "x$with_lzma" != "xno"; then
  AC_CHECK_HEADERS([lzma.h])
  AC_CHECK_LIB(lzma,lzma_stream_decoder)
fi

AC_CHECK_HEADERS([openssl/md5.h openssl/ripemd.h openssl/sha.h])
AC_CHECK_HEADERS([md5.h])
AC_CHECK_HEADERS([ripemd.h rmd160.h])
AC_CHECK_HEADERS([sha.h sha1.h sha2.h sha256.h])
AC_SEARCH_LIBS(SHA1_Init, crypto ssl md)
AC_CHECK_FUNC(MD5_Init)
if test "x$ac_cv_func_MD5_Init" != "xyes"; then
  AC_CHECK_FUNC(MD5Init,
	[AC_DEFINE(MD5_Init, MD5Init,
	    [Define to the initializes MD5 context function.])
	 AC_DEFINE(MD5_Update, MD5Update,
	    [Define to the updates MD5 context function.])
	 AC_DEFINE(MD5_Final, MD5Final,
	    [Define to the generates final MD5 hash function.])
	])
fi
if test "x$ac_cv_func_MD5_Init" = "xyes" -o "x$ac_cv_func_MD5Init" = "xyes" ; then
  AC_DEFINE(HAVE_MD5, 1, [Define to 1 if you have the `MD5' functions.])
fi
AC_CHECK_FUNC(RIPEMD160_Init)
if test "x$ac_cv_func_RIPEMD160_Init" != "xyes"; then
  AC_CHECK_FUNC(RMD160Init,
	[AC_DEFINE(RIPEMD160_Init, RMD160Init,
	    [Define to the initializes RIPEMD160 context function.])
	 AC_DEFINE(RIPEMD160_Update, RMD160Update,
	    [Define to the updates RIPEMD160 context function.])
	 AC_DEFINE(RIPEMD160_Final, RMD160Final,
	    [Define to the generates final RIPEMD160 hash function.])
	])
fi
if test "x$ac_cv_func_RIPEMD160_Init" = "xyes" -o "x$ac_cv_func_RMD160Init" = "xyes" ; then
  AC_DEFINE(HAVE_RMD160, 1, [Define to 1 if you have the `RIPEMD160' functions.])
fi
AC_CHECK_FUNC(SHA1_Init)
if test "x$ac_cv_func_SHA1_Init" != "xyes"; then
  AC_CHECK_FUNC(SHA1Init,
	[AC_DEFINE(SHA1_Init, SHA1Init,
	    [Define to the initializes SHA1 context function.])
	 AC_DEFINE(SHA1_Update, SHA1Update,
	    [Define to the updates SHA1 context function.])
	 AC_DEFINE(SHA1_Final, SHA1Final,
	    [Define to the generates final SHA1 hash function.])
	])
fi
if test "x$ac_cv_func_SHA1_Init" = "xyes" -o "x$ac_cv_func_SHA1Init" = "xyes" ; then
  AC_DEFINE(HAVE_SHA1, 1, [Define to 1 if you have the `SHA1' functions.])
fi
AC_CHECK_FUNC(SHA256_Init)
if test "x$ac_cv_func_SHA256_Init" = "xyes" ; then
  AC_DEFINE(HAVE_SHA256, 1, [Define to 1 if you have the `SHA256' functions.])
fi
AC_CHECK_FUNC(SHA384_Init)
if test "x$ac_cv_func_SHA384_Init" = "xyes" ; then
  AC_DEFINE(HAVE_SHA384, 1, [Define to 1 if you have the `SHA384' functions.])
fi
AC_CHECK_FUNC(SHA512_Init)
if test "x$ac_cv_func_SHA512_Init" = "xyes" ; then
  AC_DEFINE(HAVE_SHA512, 1, [Define to 1 if you have the `SHA512' functions.])
fi

# TODO: Give the user the option of using a pre-existing system
# libarchive.  This will define HAVE_LIBARCHIVE which will cause
# bsdtar_platform.h to use #include <...> for the libarchive headers.
# Need to include Makefile.am magic to link against system
# -larchive in that case.
#AC_CHECK_LIB(archive,archive_version)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
# AC_TYPE_OFF_T defaults to "long", which limits us to 4GB files on
# most systems... default to "long long" instead.
AC_CHECK_TYPE(off_t, [long long])
AC_TYPE_SIZE_T
AC_CHECK_TYPE(id_t, [unsigned long])
AC_CHECK_TYPE(uintptr_t, [unsigned int])

# Check for birthtime in struct stat
AC_CHECK_MEMBERS([struct stat.st_birthtime])

# Check for high-resolution timestamps in struct stat
AC_CHECK_MEMBERS([struct stat.st_birthtimespec.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_mtimespec.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_mtim.tv_nsec])
AC_CHECK_MEMBERS([struct stat.st_mtime_n]) # AIX
AC_CHECK_MEMBERS([struct stat.st_umtime]) # Tru64
AC_CHECK_MEMBERS([struct stat.st_mtime_usec]) # Hurd
# Check for block size support in struct stat
AC_CHECK_MEMBERS([struct stat.st_blksize])
# Check for st_flags in struct stat (BSD fflags)
AC_CHECK_MEMBERS([struct stat.st_flags])

# If you have uintmax_t, we assume printf supports %ju
# If you have unsigned long long, we assume printf supports %llu
# TODO: Check for %ju and %llu support directly.
AC_CHECK_TYPES([uintmax_t, unsigned long long])

# We need int64_t, uint64_t, intmax_t, and uintmax_t
AC_TYPE_INTMAX_T
AC_TYPE_INT64_T
AC_TYPE_UINTMAX_T
AC_TYPE_UINT64_T

# TODO: If any of these are missing, define them right here.
AC_CHECK_DECLS([SIZE_MAX, SSIZE_MAX, INT64_MAX, INT64_MIN, UINT64_MAX, UINT32_MAX])

AC_CHECK_DECL([EFTYPE],
		[AC_DEFINE(HAVE_EFTYPE, 1, [A possible errno value for invalid file format errors])],
		[],
		[#include <errno.h>])
AC_CHECK_DECL([EILSEQ],
		[AC_DEFINE(HAVE_EILSEQ, 1, [A possible errno value for invalid file format errors])],
		[],
		[#include <errno.h>])
AC_CHECK_TYPE([wchar_t],
	        [AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_[]wchar_t), 1, [Define to 1 if the system has the type `wchar_t'.])dnl
		AC_CHECK_SIZEOF([wchar_t])],
		[])

AC_HEADER_TIME

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_HEADER_MAJOR
AC_FUNC_FSEEKO
AC_FUNC_MEMCMP
AC_FUNC_LSTAT
AC_FUNC_STAT
AC_FUNC_STRERROR_R
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([chflags chown chroot])
AC_CHECK_FUNCS([fchdir fchflags fchmod fchown fcntl fork])
AC_CHECK_FUNCS([fstat ftruncate futimes geteuid getpid])
AC_CHECK_FUNCS([lchflags lchmod lchown])
AC_CHECK_FUNCS([lutimes memmove memset mkdir mkfifo mknod])
AC_CHECK_FUNCS([nl_langinfo pipe poll readlink select setenv setlocale])
AC_CHECK_FUNCS([strchr strdup strerror strrchr timegm])
AC_CHECK_FUNCS([tzset unsetenv utime utimes vfork])
AC_CHECK_FUNCS([wcrtomb wcscpy wcslen wctomb wmemcmp wmemcpy])

# FreeBSD's nl_langinfo supports an option to specify whether the
# current locale uses month/day or day/month ordering.  It makes the
# output a little prettier...
AC_CHECK_DECL([D_MD_ORDER],
[AC_DEFINE(HAVE_D_MD_ORDER, 1, [Define to 1 if nl_langinfo supports D_MD_ORDER])],
[],
[#if HAVE_LANGINFO_H
#include <langinfo.h>
#endif
])

# If fnmatch() exists and supports FNM_LEADING_DIR, then bsdtar uses that,
# otherwise it uses some usable (but less featureful) built-in code for
# filename pattern matching.
AC_FUNC_FNMATCH
AC_CHECK_DECL([FNM_LEADING_DIR],
[AC_DEFINE(HAVE_FNM_LEADING_DIR, 1, [Define to 1 if fnmatch(3) supports the FNM_LEADING_DIR flag])],
[],
[#if HAVE_FNMATCH
#define _GNU_SOURCE /* Required on Linux to get GNU extensions */
#include <fnmatch.h>
#endif
])

# Check for dirent.d_namlen field explicitly
# (This is a bit more straightforward than, if not quite as portable as,
# the recipe given by the autoconf maintainers.)
AC_CHECK_MEMBER(struct dirent.d_namlen,,,
[#if HAVE_DIRENT_H
#include <dirent.h>
#endif
])

# Check for Extended Attributes support
AC_ARG_ENABLE([xattr],
		AS_HELP_STRING([--disable-xattr],
		[Enable Extended Attributes support (default: check)]))

if test "x$enable_xattr" != "xno"; then
	AC_CHECK_HEADERS([attr/xattr.h])
	AC_CHECK_HEADERS([sys/xattr.h])
	AC_CHECK_LIB(attr,setxattr)
	AC_CHECK_FUNCS([extattr_get_file extattr_list_file])
	AC_CHECK_FUNCS([extattr_set_fd extattr_set_file])
	AC_CHECK_FUNCS([fsetxattr getxattr])
	AC_CHECK_FUNCS([lgetxattr listxattr llistxattr lsetxattr])
fi

# Check for ACL support
#
# The ACL support in libarchive is written against the POSIX1e draft,
# which was never officially approved and varies quite a bit across
# platforms.  Worse, some systems have completely non-POSIX acl functions,
# which makes the following checks rather more complex than I would like.
#
AC_ARG_ENABLE([acl],
		AS_HELP_STRING([--disable-acl],
		[Enable ACL support (default: check)]))

if test "x$enable_acl" != "xno"; then
   AC_CHECK_HEADERS([sys/acl.h])
   AC_CHECK_LIB([acl],[acl_get_file])
   AC_CHECK_FUNCS([acl_create_entry acl_init acl_set_fd acl_set_fd_np acl_set_file])

   AC_CHECK_TYPES(acl_permset_t,,,
	[#if HAVE_SYS_TYPES_H
	#include <sys/types.h>
	#endif
	#if HAVE_SYS_ACL_H
	#include <sys/acl.h>
	#endif
	])

    # The "acl_get_perm()" function was omitted from the POSIX draft.
    # (It's a pretty obvious oversight; otherwise, there's no way to
    # test for specific permissions in a permset.)  Linux uses the obvious
    # name, FreeBSD adds _np to mark it as "non-Posix extension."
    # Test for both as a double-check that we really have POSIX-style ACL support.
    AC_CHECK_FUNCS(acl_get_perm_np acl_get_perm,,,
	[#if HAVE_SYS_TYPES_H
	#include <sys/types.h>
	#endif
	#if HAVE_SYS_ACL_H
	#include <sys/acl.h>
	#endif
	])

    # MacOS has an acl.h that isn't POSIX.  It can be detected by
    # checking for ACL_USER
    AC_CHECK_DECL([ACL_USER],
		[AC_DEFINE(HAVE_ACL_USER, 1, [True for systems with POSIX ACL support])],
		[],
		[#include <sys/acl.h>])
fi

# Additional requirements
AC_SYS_LARGEFILE

# Interix doesn't provide optarg and optind
AC_CHECK_DECLS([optarg])
AC_CHECK_DECLS([optind])

AC_OUTPUT
