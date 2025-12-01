#
# deps.mk - dependencies for the current makefile
# 
# Automatically Generated. DO NOT EDIT
# 
# Created by makedeps.sh on Fri Mar  4 03:43:20 GMT 2011


cairo-ft.h : cairo.h FT_FREETYPE_H

cairo.h : cairo-version.h cairo-features.h cairo-deprecated.h

cairo-pdf.h : cairo.h

cairo-ps.h : cairo.h

cairo-svg.h : cairo.h

cairo-win32.h : cairo.h