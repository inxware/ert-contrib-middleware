#
# deps.mk - dependencies for the current makefile
# 
# Automatically Generated. DO NOT EDIT
# 
# Created by makedeps.sh on Sun 17 Apr 23:35:17 BST 2022


archive.h : __LA_STDINT_H

expat.h : expat_external.h

faad.h : neaacdec.h

jpeglib.h : jconfig.h jmorecfg.h jpegint.h jerror.h

mp4ff.h : mp4ff_int_types.h

mp4ffint.h : mp4ff_int_types.h ../../config.h

pngconf.h : config.h pngusr.h

png.h : zlib.h pngconf.h

sqlite3ext.h : sqlite3.h

tiff.h : tiffconf.h

tiffio.h : tiff.h tiffvers.h

zlib.h : zconf.h