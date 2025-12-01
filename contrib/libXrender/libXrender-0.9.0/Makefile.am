# 
#  $Id: Makefile.am,v 1.11 2004/07/29 07:59:34 keithp Exp $
# 
#  Copyright © 2003 Keith Packard, Noah Levitt
# 
#  Permission to use, copy, modify, distribute, and sell this software and its
#  documentation for any purpose is hereby granted without fee, provided that
#  the above copyright notice appear in all copies and that both that
#  copyright notice and this permission notice appear in supporting
#  documentation, and that the name of Keith Packard not be used in
#  advertising or publicity pertaining to distribution of the software without
#  specific, written prior permission.  Keith Packard makes no
#  representations about the suitability of this software for any purpose.  It
#  is provided "as is" without express or implied warranty.
# 
#  KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
#  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
#  EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
#  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
#  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
#  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
#  PERFORMANCE OF THIS SOFTWARE.

AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = $(RENDER_CFLAGS) $(X_CFLAGS)

lib_LTLIBRARIES = libXrender.la

libXrender_la_SOURCES = AddTrap.c \
                        Color.c \
                        Composite.c \
                        Cursor.c \
                        FillRect.c \
                        FillRects.c \
                        Filter.c \
                        Glyph.c \
                        Picture.c \
                        Poly.c \
                        Trap.c \
                        Tri.c \
                        Xrender.c \
                        Xrender.h \
                        Xrenderint.h \
                        region.h

libXrender_la_LIBADD = @X_LIBS@ -lX11

# -version-number requires libtool >= 1.5
libXrender_la_LDFLAGS = -version-number 1:3:0 -no-undefined

libXrenderincludedir = $(includedir)/X11/extensions
libXrenderinclude_HEADERS = Xrender.h 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xrender.pc

EXTRA_DIST = xrender.pc.in autogen.sh
