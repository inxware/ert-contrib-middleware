#
# deps.mk - dependencies for the current makefile
# 
# Automatically Generated. DO NOT EDIT
# 
# Created by makedeps.sh on Fri Mar  4 03:43:20 GMT 2011


control.h : rpc.h rpcndr.h

d3dcaps.h : ddraw.h

d3d.h : d3dtypes.h d3dcaps.h

d3drmdef.h : d3dtypes.h

d3drm.h : ddraw.h d3drmobj.h

d3drmobj.h : d3drmdef.h d3d.h

d3drmwin.h : d3drm.h ddraw.h d3d.h

d3dtypes.h : ddraw.h d3dvec.inl

d3dxcore.h : d3dxerr.h

d3dx.h : d3dxcore.h d3dxmath.h d3dxshapes.h d3dxsprite.h

d3dxmath.h : d3dxerr.h d3dxmath.inl

d3dxshapes.h : d3dxerr.h

d3dxsprite.h : d3dxerr.h

dmdls.h : dls1.h

dmusicc.h : dls1.h dmerror.h dmdls.h dsound.h dmusbuff.h

dmusici.h : dmusicc.h

dplobby.h : dplay.h

dsound.h : d3dtypes.h

faad.h : neaacdec.h

fluidsynth.h : fluidsynth/types.h fluidsynth/settings.h fluidsynth/synth.h fluidsynth/shell.h fluidsynth/sfont.h fluidsynth/ramsfont.h fluidsynth/audio.h fluidsynth/event.h fluidsynth/midi.h fluidsynth/seq.h fluidsynth/seqbind.h fluidsynth/log.h fluidsynth/misc.h fluidsynth/mod.h fluidsynth/gen.h fluidsynth/voice.h fluidsynth/version.h

gsm.h : <stdio.h>

iconv.h : iconv.h

jpeglib.h : jconfig.h jmorecfg.h jpegint.h jerror.h

lauxlib.h : lua.h

lua.h : luaconf.h LUA_USER_H

lualib.h : lua.h

NetCommon.h : IMN_PIMNetCommon.h

pngconf.h : pngusr.h config.h

png.h : zlib.h pngconf.h

pthread.h : config.h need_errno.h

sched.h : need_errno.h

semaphore.h : need_errno.h

sqlite3ext.h : sqlite3.h

strmif.h : rpc.h rpcndr.h windows.h ole2.h unknwn.h objidl.h oaidl.h ocidl.h

tiff.h : tiffconf.h

tiffio.h : tiff.h tiffvers.h

uuids.h : ksuuids.h

zlib.h : zconf.h