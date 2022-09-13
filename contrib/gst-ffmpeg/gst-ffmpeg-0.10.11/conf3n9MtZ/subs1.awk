BEGIN {
S["am__EXEEXT_FALSE"]=""
S["am__EXEEXT_TRUE"]="#"
S["LTLIBOBJS"]=""
S["LIBOBJS"]=""
S["HAVE_FFMPEG_UNINSTALLED_FALSE"]="#"
S["HAVE_FFMPEG_UNINSTALLED_TRUE"]=""
S["WIN32_LIBS"]=""
S["FFMPEG_SUBDIRS"]="gst-libs"
S["FFMPEG_EXTERNALS_REVISION"]=""
S["FFMPEG_REVISION"]="23623"
S["FFMPEG_SVN"]="svn://svn.ffmpeg.org/ffmpeg/branches/0.6"
S["FFMPEG_CO_DIR"]="gst-libs/ext/ffmpeg"
S["SWSCALE_LIBS"]="$(top_builddir)/gst-libs/ext/ffmpeg/libswscale/libswscale.a                 $(top_builddir)/gst-libs/ext/ffmpeg/libavutil/libavutil.a"
S["SWSCALE_CFLAGS"]="-I $(top_srcdir)/gst-libs/ext/ffmpeg/libswscale	                  -I $(top_srcdir)/gst-libs/ext/ffmpeg/libavutil 	                  -I $(top_srcdir)"\
"/gst-libs/ext/ffmpeg 			   -I $(top_builddir)/gst-libs/ext/ffmpeg                   -Wno-deprecated-declarations"
S["POSTPROC_LIBS"]="$(top_builddir)/gst-libs/ext/ffmpeg/libpostproc/libpostproc.a   		 $(top_builddir)/gst-libs/ext/ffmpeg/libavutil/libavutil.a"
S["POSTPROC_CFLAGS"]="-I $(top_srcdir)/gst-libs/ext/ffmpeg/libpostproc	                   -I $(top_srcdir)/gst-libs/ext/ffmpeg/libavutil 	                   -I $(top_srcd"\
"ir)/gst-libs/ext/ffmpeg/libavcodec 	                   -I $(top_srcdir)/gst-libs/ext/ffmpeg 			   -I $(top_builddir)/gst-libs/ext/ffmpeg            "\
"        -Wno-deprecated-declarations"
S["FFMPEG_LIBS"]="$(top_builddir)/gst-libs/ext/ffmpeg/libavformat/libavformat.a                $(top_builddir)/gst-libs/ext/ffmpeg/libavcodec/libavcodec.a            "\
"    $(top_builddir)/gst-libs/ext/ffmpeg/libavutil/libavutil.a"
S["FFMPEG_CFLAGS"]="-I $(top_srcdir)/gst-libs/ext/ffmpeg/libavformat                  -I $(top_srcdir)/gst-libs/ext/ffmpeg/libavutil                  -I $(top_srcdir)/g"\
"st-libs/ext/ffmpeg/libavcodec 		 -I $(top_srcdir)/gst-libs/ext/ffmpeg 		 -I $(top_builddir)/gst-libs/ext/ffmpeg                  -Wno-deprecated-dec"\
"larations"
S["HAVE_BZ2_FALSE"]=""
S["HAVE_BZ2_TRUE"]="#"
S["HAVE_BZ2"]="no"
S["DARWIN_LDFLAGS"]=""
S["GST_PLUGIN_LDFLAGS"]="-module -avoid-version -export-symbols-regex '^_*gst_plugin_desc$$' -no-undefined"
S["GST_ALL_LDFLAGS"]="-no-undefined"
S["GST_OPTION_CFLAGS"]="$(WARNING_CFLAGS) $(ERROR_CFLAGS) $(DEBUG_CFLAGS) $(PROFILE_CFLAGS) $(GCOV_CFLAGS) $(OPT_CFLAGS) $(DEPRECATED_CFLAGS)"
S["DEPRECATED_CFLAGS"]=""
S["PROFILE_CFLAGS"]=""
S["GST_LEVEL_DEFAULT"]="GST_LEVEL_NONE"
S["ERROR_CFLAGS"]=""
S["WARNING_CFLAGS"]=" -Wall -Wdeclaration-after-statement -Wvla -Wpointer-arith -Wmissing-declarations -Wmissing-prototypes -Wredundant-decls -Wundef -Wwrite-strings -Wf"\
"ormat-nonliteral -Wformat-security -Wold-style-definition -Wcast-align -Winit-self -Wmissing-include-dirs -Waddress -Waggregate-return -Wno-multicha"\
"r -Wnested-externs"
S["plugindir"]="$(libdir)/gstreamer-0.10"
S["PLUGINDIR"]="/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-gtk_gst/buil"\
"d/lib/gstreamer-0.10"
S["HAVE_ORC_FALSE"]=""
S["HAVE_ORC_TRUE"]="#"
S["ORCC"]=""
S["ORC_LIBS"]=""
S["ORC_CFLAGS"]=""
S["HAVE_GST_CHECK_FALSE"]="#"
S["HAVE_GST_CHECK_TRUE"]=""
S["GST_CHECK_LIBS"]="-pthread -L/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-g"\
"tk_gst/build/lib -lgstcheck-0.10 -lm -lgstreamer-0.10 -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0  "
S["GST_CHECK_CFLAGS"]="-pthread -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-g"\
"tk_gst/build/include/gstreamer-0.10 -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gn"\
"u-i686-pc-linux-gnu-4.4.6-gtk_gst/build/include/glib-2.0 -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../targ"\
"et_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-gtk_gst/build/lib/glib-2.0/include  "
S["GSTPB_PLUGINS_DIR"]="/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-gtk_gst/buil"\
"d/lib/gstreamer-0.10"
S["GST_PLUGINS_BASE_LIBS"]="-pthread -L/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-g"\
"tk_gst/build/lib -lgstreamer-0.10 -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0  "
S["GST_PLUGINS_BASE_CFLAGS"]="-pthread -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-g"\
"tk_gst/build/include/gstreamer-0.10 -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gn"\
"u-i686-pc-linux-gnu-4.4.6-gtk_gst/build/include/glib-2.0 -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../targ"\
"et_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-gtk_gst/build/lib/glib-2.0/include  "
S["GST_BASE_LIBS"]="-pthread -L/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-g"\
"tk_gst/build/lib -lgstbase-0.10 -lgstreamer-0.10 -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0  "
S["GST_BASE_CFLAGS"]="-pthread -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-g"\
"tk_gst/build/include/gstreamer-0.10 -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gn"\
"u-i686-pc-linux-gnu-4.4.6-gtk_gst/build/include/glib-2.0 -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../targ"\
"et_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-gtk_gst/build/lib/glib-2.0/include  "
S["GST_PLUGINS_DIR"]="/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-gtk_gst/buil"\
"d/lib/gstreamer-0.10"
S["GST_TOOLS_DIR"]="/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-gtk_gst/buil"\
"d/bin"
S["GST_LIBS"]="-pthread -L/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-g"\
"tk_gst/build/lib -lgstreamer-0.10 -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0  "
S["GST_CFLAGS"]="-I$(top_srcdir)/gst-libs -I$(top_builddir)/gst-libs -pthread -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../"\
"target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-gtk_gst/build/include/gstreamer-0.10 -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-su"\
"pport/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-gtk_gst/build/include/glib-2.0 -I/home/pdrezet/workspace/inxware2.0-Re"\
"lease/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-gtk_gst/build/lib/glib-2.0/include    $(GST_OPTIO"\
"N_CFLAGS)"
S["LIBM"]="-lm"
S["ENABLE_PLUGIN_DOCS_FALSE"]=""
S["ENABLE_PLUGIN_DOCS_TRUE"]="#"
S["pkgpyexecdir"]="${pyexecdir}/gst-ffmpeg"
S["pyexecdir"]="${exec_prefix}/lib/python2.6/site-packages"
S["pkgpythondir"]="${pythondir}/gst-ffmpeg"
S["pythondir"]="${prefix}/lib/python2.6/site-packages"
S["PYTHON_PLATFORM"]="linux2"
S["PYTHON_EXEC_PREFIX"]="${exec_prefix}"
S["PYTHON_PREFIX"]="${prefix}"
S["PYTHON_VERSION"]="2.6"
S["PYTHON"]="/usr/bin/python"
S["GTK_DOC_USE_LIBTOOL_FALSE"]="#"
S["GTK_DOC_USE_LIBTOOL_TRUE"]=""
S["ENABLE_GTK_DOC_FALSE"]=""
S["ENABLE_GTK_DOC_TRUE"]="#"
S["GTKDOC_CHECK"]=""
S["HTML_DIR"]="${datadir}/gtk-doc/html"
S["DOC_PS_FALSE"]=""
S["DOC_PS_TRUE"]="#"
S["DOC_PDF_FALSE"]=""
S["DOC_PDF_TRUE"]="#"
S["DOC_HTML_FALSE"]=""
S["DOC_HTML_TRUE"]="#"
S["ENABLE_DOCBOOK_FALSE"]=""
S["ENABLE_DOCBOOK_TRUE"]="#"
S["HAVE_EPSTOPDF"]=""
S["HAVE_PNMTOPS"]=""
S["HAVE_PNGTOPNM"]=""
S["HAVE_FIG2DEV"]=""
S["HAVE_XMLLINT"]=""
S["HAVE_DVIPS"]=""
S["CAT_ENTRY_END"]=""
S["CAT_ENTRY_START"]=""
S["DOCBOOK_ROOT"]=""
S["XSLTPROC_FLAGS"]=""
S["XML_CATALOG"]=""
S["XSLTPROC"]=""
S["HAVE_PS2PDF"]=""
S["HAVE_JADETEX"]=""
S["HAVE_DOCBOOK2HTML"]=""
S["HAVE_DOCBOOK2PS"]=""
S["HAVE_VALGRIND_FALSE"]=""
S["HAVE_VALGRIND_TRUE"]="#"
S["VALGRIND_PATH"]="no"
S["HAVE_CPU_CRISV32_FALSE"]=""
S["HAVE_CPU_CRISV32_TRUE"]="#"
S["HAVE_CPU_CRIS_FALSE"]=""
S["HAVE_CPU_CRIS_TRUE"]="#"
S["HAVE_CPU_X86_64_FALSE"]=""
S["HAVE_CPU_X86_64_TRUE"]="#"
S["HAVE_CPU_M68K_FALSE"]=""
S["HAVE_CPU_M68K_TRUE"]="#"
S["HAVE_CPU_IA64_FALSE"]=""
S["HAVE_CPU_IA64_TRUE"]="#"
S["HAVE_CPU_S390_FALSE"]=""
S["HAVE_CPU_S390_TRUE"]="#"
S["HAVE_CPU_MIPS_FALSE"]=""
S["HAVE_CPU_MIPS_TRUE"]="#"
S["HAVE_CPU_HPPA_FALSE"]=""
S["HAVE_CPU_HPPA_TRUE"]="#"
S["HAVE_CPU_SPARC_FALSE"]=""
S["HAVE_CPU_SPARC_TRUE"]="#"
S["HAVE_CPU_ARM_FALSE"]=""
S["HAVE_CPU_ARM_TRUE"]="#"
S["HAVE_CPU_ALPHA_FALSE"]=""
S["HAVE_CPU_ALPHA_TRUE"]="#"
S["HAVE_CPU_PPC64_FALSE"]=""
S["HAVE_CPU_PPC64_TRUE"]="#"
S["HAVE_CPU_PPC_FALSE"]=""
S["HAVE_CPU_PPC_TRUE"]="#"
S["HAVE_CPU_I386_FALSE"]="#"
S["HAVE_CPU_I386_TRUE"]=""
S["VALGRIND_LIBS"]=""
S["VALGRIND_CFLAGS"]=""
S["PKG_CONFIG"]="/usr/bin/pkg-config"
S["GST_PACKAGE_ORIGIN"]="Unknown package origin"
S["GST_PACKAGE_NAME"]="GStreamer FFMpeg source release"
S["ACLOCAL_AMFLAGS"]="-I m4 -I common/m4"
S["CPP"]="i686-pc-linux-gnu-gcc -E"
S["OTOOL64"]=""
S["OTOOL"]=""
S["LIPO"]=""
S["NMEDIT"]=""
S["DSYMUTIL"]=""
S["RANLIB"]="i686-pc-linux-gnu-ranlib"
S["AR"]="i686-pc-linux-gnu-ar"
S["LN_S"]="ln -s"
S["NM"]="nm"
S["ac_ct_DUMPBIN"]=""
S["DUMPBIN"]=""
S["LD"]="/home/pdrezet/workspace/inxware2.0-Release/INX/EHS-build-support/toolchains/i686-pc-linux-gnu-4.4.6/i686-pc-linux-gnu/bin/ld"
S["FGREP"]="/bin/grep -F"
S["EGREP"]="/bin/grep -E"
S["GREP"]="/bin/grep"
S["SED"]="/bin/sed"
S["am__fastdepCC_FALSE"]="#"
S["am__fastdepCC_TRUE"]=""
S["CCDEPMODE"]="depmode=gcc3"
S["AMDEPBACKSLASH"]="\\"
S["AMDEP_FALSE"]="#"
S["AMDEP_TRUE"]=""
S["am__quote"]=""
S["am__include"]="include"
S["DEPDIR"]=".deps"
S["OBJEXT"]="o"
S["EXEEXT"]=""
S["ac_ct_CC"]=""
S["CPPFLAGS"]=""
S["LDFLAGS"]=" --sysroot=/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../../EHS-build-support/toolchains/i686-pc-linux-gnu-4."\
"4.6/sysroot -L/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4."\
"6-gtk_gst/build/lib/  -L/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../../EHS-build-support/support_libs/targe"\
"t_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6/build/lib -L/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../../EH"\
"S-build-support/toolchains/i686-pc-linux-gnu-4.4.6/sysroot/usr/lib -lc_nonshared"
S["CFLAGS"]="--sysroot=/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../../EHS-build-support/toolchains/i686-pc-linux-gnu-4.4"\
".6/sysroot -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6"\
"-gtk_gst/build/include/ -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../../EHS-build-support/support_libs/tar"\
"get_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6/build/include/ -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/."\
"./../EHS-build-support/kernel-dependencies/linux/2.6.35.9 -I/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../../"\
"EHS-build-support/toolchains/i686-pc-linux-gnu-4.4.6/sysroot/usr/include "
S["CC"]="i686-pc-linux-gnu-gcc"
S["LIBTOOL"]="$(SHELL) $(top_builddir)/libtool"
S["OBJDUMP"]="objdump"
S["DLLTOOL"]="dlltool"
S["AS"]="i686-pc-linux-gnu-gcc"
S["GST_MAJORMINOR"]="0.10"
S["AM_BACKSLASH"]="\\"
S["AM_DEFAULT_VERBOSITY"]="0"
S["host_os"]="linux-gnu"
S["host_vendor"]="pc"
S["host_cpu"]="i686"
S["host"]="i686-pc-linux-gnu"
S["build_os"]="linux-gnu"
S["build_vendor"]="pc"
S["build_cpu"]="i386"
S["build"]="i386-pc-linux-gnu"
S["MAINT"]="#"
S["MAINTAINER_MODE_FALSE"]=""
S["MAINTAINER_MODE_TRUE"]="#"
S["PACKAGE_VERSION_RELEASE"]="1"
S["PACKAGE_VERSION_NANO"]="0"
S["PACKAGE_VERSION_MICRO"]="11"
S["PACKAGE_VERSION_MINOR"]="10"
S["PACKAGE_VERSION_MAJOR"]="0"
S["am__untar"]="${AMTAR} xf -"
S["am__tar"]="${AMTAR} chof - \"$$tardir\""
S["AMTAR"]="${SHELL} /home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/contrib/gst-ffmpeg/gst-ffmpeg-0.10.11/missing --run tar"
S["am__leading_dot"]="."
S["SET_MAKE"]=""
S["AWK"]="gawk"
S["mkdir_p"]="/bin/mkdir -p"
S["MKDIR_P"]="/bin/mkdir -p"
S["INSTALL_STRIP_PROGRAM"]="$(install_sh) -c -s"
S["STRIP"]="strip"
S["install_sh"]="${SHELL} /home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/contrib/gst-ffmpeg/gst-ffmpeg-0.10.11/install-sh"
S["MAKEINFO"]="${SHELL} /home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/contrib/gst-ffmpeg/gst-ffmpeg-0.10.11/missing --run makeinfo"
S["AUTOHEADER"]="${SHELL} /home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/contrib/gst-ffmpeg/gst-ffmpeg-0.10.11/missing --run autoheader"
S["AUTOMAKE"]="${SHELL} /home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/contrib/gst-ffmpeg/gst-ffmpeg-0.10.11/missing --run automake-1.11"
S["AUTOCONF"]="${SHELL} /home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/contrib/gst-ffmpeg/gst-ffmpeg-0.10.11/missing --run autoconf"
S["ACLOCAL"]="${SHELL} /home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/contrib/gst-ffmpeg/gst-ffmpeg-0.10.11/missing --run aclocal-1.11"
S["VERSION"]="0.10.11"
S["PACKAGE"]="gst-ffmpeg"
S["CYGPATH_W"]="echo"
S["am__isrc"]=""
S["INSTALL_DATA"]="${INSTALL} -m 644"
S["INSTALL_SCRIPT"]="${INSTALL}"
S["INSTALL_PROGRAM"]="${INSTALL}"
S["target_alias"]=""
S["host_alias"]="i686-linux-gnu"
S["build_alias"]="i386-linux"
S["LIBS"]=""
S["ECHO_T"]=""
S["ECHO_N"]="-n"
S["ECHO_C"]=""
S["DEFS"]="-DHAVE_CONFIG_H"
S["mandir"]="${datarootdir}/man"
S["localedir"]="${datarootdir}/locale"
S["libdir"]="${exec_prefix}/lib"
S["psdir"]="${docdir}"
S["pdfdir"]="${docdir}"
S["dvidir"]="${docdir}"
S["htmldir"]="${docdir}"
S["infodir"]="${datarootdir}/info"
S["docdir"]="$(datadir)/doc/gst-ffmpeg-0.10"
S["oldincludedir"]="/usr/include"
S["includedir"]="${prefix}/include"
S["localstatedir"]="${prefix}/var"
S["sharedstatedir"]="${prefix}/com"
S["sysconfdir"]="${prefix}/etc"
S["datadir"]="${datarootdir}"
S["datarootdir"]="${prefix}/share"
S["libexecdir"]="${exec_prefix}/libexec"
S["sbindir"]="${exec_prefix}/sbin"
S["bindir"]="${exec_prefix}/bin"
S["program_transform_name"]="s,x,x,"
S["prefix"]="/home/pdrezet/workspace/inxware2.0-Release/INX/comp-lib-support/inx_build_scripts/../target_libs/i686-linux-gnu-i686-pc-linux-gnu-4.4.6-gtk_gst/buil"\
"d"
S["exec_prefix"]="${prefix}"
S["PACKAGE_URL"]=""
S["PACKAGE_BUGREPORT"]="http://bugzilla.gnome.org/enter_bug.cgi?product=GStreamer"
S["PACKAGE_STRING"]="GStreamer FFMpeg 0.10.11"
S["PACKAGE_VERSION"]="0.10.11"
S["PACKAGE_TARNAME"]="gst-ffmpeg"
S["PACKAGE_NAME"]="GStreamer FFMpeg"
S["PATH_SEPARATOR"]=":"
S["SHELL"]="/bin/sh"
  for (key in S) S_is_set[key] = 1
  FS = ""

}
{
  line = $ 0
  nfields = split(line, field, "@")
  substed = 0
  len = length(field[1])
  for (i = 2; i < nfields; i++) {
    key = field[i]
    keylen = length(key)
    if (S_is_set[key]) {
      value = S[key]
      line = substr(line, 1, len) "" value "" substr(line, len + keylen + 3)
      len += length(value) + length(field[++i])
      substed = 1
    } else
      len += 1 + keylen
  }

  print line
}

