%define majorminor  0.10
%define gstreamer   gstreamer

%define gst_minver   0.10.0

Name: 		%{gstreamer}-plugins-bad
Version: 	0.10.20
Release: 	1.gst
Summary: 	GStreamer plug-ins of bad quality

%define 	majorminor	0.10

Group: 		Applications/Multimedia
License: 	LGPL
URL:		http://gstreamer.freedesktop.org/
Vendor:         GStreamer Backpackers Team <package@gstreamer.freedesktop.org>
Source:         http://gstreamer.freedesktop.org/src/gst-plugins-bad/gst-plugins-bad-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires:         %{gstreamer} >= %{gst_minver}
BuildRequires:    %{gstreamer}-devel >= %{gst_minver}

BuildRequires:  gcc-c++
#BuildRequires: ladspa-devel
#BuildRequires:  faad2-devel >= 2.0
#BuildRequires:  gsm-devel >= 1.0.10
#Requires:      SDL >= 1.2.0
#BuildRequires:  swfdec-devel
#Provides:      gstreamer-faad = %{version}-%{release}
#Requires:      faac >= 1.23
#Provides:       gstreamer-gsm = %{version}-%{release}
#Requires: libmms >= 0.1
#Requires: gmyth

%description
GStreamer is a streaming media framework, based on graphs of filters which
operate on media data. Applications using this library can do anything
from real-time sound processing to playing videos, and just about anything
else media-related.  Its plugin-based architecture means that new data
types or processing capabilities can be added simply by installing new
plug-ins.

This package contains GStreamer Plugins that are considered to be of bad
quality, even though they might work.

%prep
%setup -q -n gst-plugins-bad-%{version}

%build
%configure --enable-experimental

make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT

%makeinstall
                                                                                
# Clean out files that should not be part of the rpm.
rm -f $RPM_BUILD_ROOT%{_libdir}/gstreamer-%{majorminor}/*.la
rm -f $RPM_BUILD_ROOT%{_libdir}/gstreamer-%{majorminor}/*.a
rm -f $RPM_BUILD_ROOT%{_libdir}/*.a
rm -f $RPM_BUILD_ROOT%{_libdir}/*.la

%find_lang gst-plugins-bad-%{majorminor}

%clean
rm -rf $RPM_BUILD_ROOT

%files -f gst-plugins-bad-%{majorminor}.lang
%defattr(-, root, root)
%doc AUTHORS COPYING README REQUIREMENTS gst-plugins-bad.doap
%{_bindir}/gst-camera
%{_bindir}/gst-camera-perf

# non-core plugins without external dependencies
%{_libdir}/gstreamer-%{majorminor}/libgsttta.so
%{_libdir}/gstreamer-%{majorminor}/libgstspeed.so
%{_libdir}/gstreamer-%{majorminor}/libgstcdxaparse.so
%{_libdir}/gstreamer-%{majorminor}/libgstfreeze.so
%{_libdir}/gstreamer-%{majorminor}/libgsth264parse.so
%{_libdir}/gstreamer-%{majorminor}/libgstnsf.so
%{_libdir}/gstreamer-%{majorminor}/libgstnuvdemux.so
%{_libdir}/gstreamer-%{majorminor}/libgstrfbsrc.so
%{_libdir}/gstreamer-%{majorminor}/libgstreal.so
%{_libdir}/gstreamer-%{majorminor}/libgstmve.so
%{_libdir}/gstreamer-%{majorminor}/libgstmpegvideoparse.so
%{_libdir}/gstreamer-%{majorminor}/libgstbayer.so
%{_libdir}/gstreamer-%{majorminor}/libgstdvdspu.so
%{_libdir}/gstreamer-%{majorminor}/libgstfestival.so
%{_libdir}/gstreamer-%{majorminor}/libgststereo.so
%{_libdir}/gstreamer-%{majorminor}/libgstvcdsrc.so
%{_libdir}/gstreamer-%{majorminor}/libgstdvb.so
%{_libdir}/gstreamer-%{majorminor}/libgstsdpelem.so
%{_libdir}/gstreamer-%{majorminor}/libgstmpeg4videoparse.so
%{_libdir}/gstreamer-%{majorminor}/libgstfbdevsink.so
%{_libdir}/gstreamer-%{majorminor}/libgstrawparse.so
%{_libdir}/gstreamer-%{majorminor}/libgstselector.so
%{_libdir}/gstreamer-%{majorminor}/libgstsubenc.so
%{_libdir}/gstreamer-%{majorminor}/libresindvd.so
%{_libdir}/gstreamer-%{majorminor}/libgstaiff.so
%{_libdir}/gstreamer-%{majorminor}/libgstdccp.so
%{_libdir}/gstreamer-%{majorminor}/libgstpcapparse.so
%{_libdir}/gstreamer-%{majorminor}/libgstmpegtsmux.so
%{_libdir}/gstreamer-%{majorminor}/libgstscaletempoplugin.so
%{_libdir}/gstreamer-%{majorminor}/libgstmpegdemux.so
%{_libdir}/gstreamer-%{majorminor}/libgstjp2k.so
%{_libdir}/gstreamer-%{majorminor}/libgstapexsink.so
%{_libdir}/gstreamer-%{majorminor}/libgstqtmux.so
%{_libdir}/gstreamer-%{majorminor}/libgstlegacyresample.so
%{_libdir}/gstreamer-%{majorminor}/libgstmxf.so
%{_libdir}/gstreamer-%{majorminor}/libgstvmnc.so
%{_libdir}/gstreamer-%{majorminor}/libgstvideosignal.so
%{_libdir}/gstreamer-%{majorminor}/libgstvalve.so
%{_libdir}/gstreamer-%{majorminor}/libgstautoconvert.so
%{_libdir}/gstreamer-%{majorminor}/libgstdtmf.so
%{_libdir}/gstreamer-%{majorminor}/libgstliveadder.so
%{_libdir}/gstreamer-%{majorminor}/libgstrtpmux.so
%{_libdir}/gstreamer-%{majorminor}/libgstsiren.so
%{_libdir}/gstreamer-%{majorminor}/libgstadpcmdec.so
%{_libdir}/gstreamer-%{majorminor}/libgstadpcmenc.so
%{_libdir}/gstreamer-%{majorminor}/libgstid3tag.so
%{_libdir}/gstreamer-%{majorminor}/libgsthdvparse.so
%{_libdir}/gstreamer-%{majorminor}/libgstdebugutilsbad.so
%{_libdir}/gstreamer-%{majorminor}/libgstasfmux.so
%{_libdir}/gstreamer-%{majorminor}/libgstpnm.so
%{_libdir}/gstreamer-%{majorminor}/libgstvideomeasure.so
%{_libdir}/gstreamer-%{majorminor}/libgstaudioparsersbad.so
%{_libdir}/gstreamer-%{majorminor}/libgstrsvg.so

%{_includedir}/gstreamer-%{majorminor}/gst/video/gstbasevideocodec.h
%{_includedir}/gstreamer-%{majorminor}/gst/video/gstbasevideodecoder.h
%{_includedir}/gstreamer-%{majorminor}/gst/video/gstbasevideoencoder.h
%{_includedir}/gstreamer-%{majorminor}/gst/video/gstbasevideoparse.h
%{_includedir}/gstreamer-%{majorminor}/gst/video/gstbasevideoutils.h
%{_datadir}/gstreamer-%{majorminor}/camera-apps/gst-camera.ui
%{_includedir}/gstreamer-%{majorminor}/gst/signalprocessor/gstsignalprocessor.h

%{_includedir}/gstreamer-%{majorminor}/gst/interfaces/photography-enumtypes.h
%{_includedir}/gstreamer-%{majorminor}/gst/interfaces/photography.h
%{_libdir}/libgstphotography-0.10.so
%{_libdir}/gstreamer-%{majorminor}/libgstcamerabin.so
%{_libdir}/libgstphotography-%{majorminor}.so.0
%{_libdir}/libgstphotography-%{majorminor}.so.0.0.0
%{_libdir}/libgstbasevideo*
%{_libdir}/libgstsignalprocessor*
%{_libdir}/gstreamer-%{majorminor}/libgstmpegpsmux.so

# hopefully very shortlived .pc file for bad
%{_libdir}/pkgconfig/gstreamer-plugins-bad-0.10.pc

# gstreamer-plugins with external dependencies but in the main package
#%{_libdir}/gstreamer-%{majorminor}/libgstfaad.so
#%{_libdir}/gstreamer-%{majorminor}/libgstfaac.so
#%{_libdir}/gstreamer-%{majorminor}/libgsttrm.so
#%{_libdir}/gstreamer-%{majorminor}/libgstsdl.so
#%{_libdir}/gstreamer-%{majorminor}/libgstswfdec.so
#%{_libdir}/gstreamer-%{majorminor}/libgstmms.so
#%{_libdir}/gstreamer-%{majorminor}/libgstxvid.so
#%{_libdir}/gstreamer-%{majorminor}/libgstbz2.so
#%{_libdir}/gstreamer-%{majorminor}/libgstneonhttpsrc.so
#%{_libdir}/gstreamer-%{majorminor}/libgstalsaspdif.so
#%{_libdir}/gstreamer-%{majorminor}/libgstmusepack.so
#%{_libdir}/gstreamer-%{majorminor}/libgstgsm.so
#%{_libdir}/gstreamer-%{majorminor}/libgstdtsdec.so
#%{_libdir}/gstreamer-%{majorminor}/libgstladspa.so
#%{_libdir}/gstreamer-%{majorminor}/libgstmythtvsrc.so
#%{_libdir}/gstreamer-%{majorminor}/libgstdc1394.so
#%{_libdir}/gstreamer-%{majorminor}/libgsttimidity.so
#%{_libdir}/gstreamer-%{majorminor}/libgstwildmidi.so
#%{_libdir}/gstreamer-%{majorminor}/libgstjack.so
#%{_libdir}/gstreamer-%{majorminor}/libgstsndfile.so
#%{_libdir}/gstreamer-%{majorminor}/libgstcelt.so
#%{_libdir}/gstreamer-%{majorminor}/libgstmpeg2enc.so
#%{_libdir}/gstreamer-%{majorminor}/libgstmplex.so
#%{_libdir}/gstreamer-%{majorminor}/libgstkate.so
#%{_libdir}/gstreamer-%{majorminor}/libgstassrender.so
%{_libdir}/gstreamer-%{majorminor}/libgstfrei0r.so
#%{_libdir}/gstreamer-%{majorminor}/libgstschro.so
#%{_libdir}/gstreamer-%{majorminor}/libgstofa.so
#%{_libdir}/gstreamer-%{majorminor}/libgstmetadata.so

%changelog
* Thu Mar 12 2009 Christian Schaller <chrisian.schaller at collabora dot co uk>
- Add Celt, mpeg2enc and mplex plugins to spec file

* Thu Oct 9 2008 Christian Schaller <chrisian.schaller at collabora dot co uk>
- flacparse, flvmux and j2kdec plugins added

* Mon Sep 1 2008 Christian Schaller <christian.schaller at collabora dot co uk>
- Add tsmux and scaletempo plugins

* Fri May 2 2008 Christian Schaller <christian.schaller at collabora dot co uk>
- Add Wildmidi plugin

* Mon Apr 14 2008 Tim-Philipp Müller <tim.muller at collabora dot co uk>
- Remove souphttpsrc plugin, which has moved to gst-plugins-good.

* Thu Apr 3 2008 Christian Schaller <christian.schaller at collabora dot co uk>
- Add new OSSv4 plugin to SPEC file

* Tue Apr 1 2008 Tim-Philipp Müller <tim.muller at collabora dot co uk>
- Update spec file for srtenc plugin rename to subenc

* Tue Apr 1 2008 Christian Schaller <christian.schaller at collabora dot co uk>
- Update spec with libgstsrtenc plugin

* Wed Jan 23 2008 Christian Schaller <christian.schaller at collabora dot co uk>
- Update spec with fbdev sink and rawparse, remove videoparse

* Fri Dec 14 2007 Christian Schaller <christian.schaller at collabora dot co uk>
- Update spec file with timidity, libgstdvb, libgstsdpelem, libgstspeexresample, libgstmpeg4videoparse

* Tue Jun 12 2007 Jan Schmidt <jan at fluendo dot com>
- wavpack and qtdemux have moved to good.

* Thu Mar 22 2007 Christian Schaller <christian at fluendo dot com>
- Add x264 and mpegvideoparse plugins

* Fri Dec 15 2006 Thomas Vander Stichele <thomas at apestaart dot org>
- add doap file
- more cleanup

* Sun Nov 27 2005 Thomas Vander Stichele <thomas at apestaart dot org>
- redone for split

