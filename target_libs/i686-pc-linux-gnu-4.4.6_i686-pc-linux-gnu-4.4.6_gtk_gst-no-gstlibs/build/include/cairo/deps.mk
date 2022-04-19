#
# deps.mk - dependencies for the current makefile
# 
# Automatically Generated. DO NOT EDIT
# 
# Created by makedeps.sh on Sun 17 Apr 23:35:18 BST 2022


cairo-ft.h : cairo.h FT_FREETYPE_H

cairo.h : cairo-version.h cairo-features.h cairo-deprecated.h

cairo-pdf.h : cairo.h

cairo-ps.h : cairo.h

cairo-svg.h : cairo.h

cairo-xlib.h : cairo.h