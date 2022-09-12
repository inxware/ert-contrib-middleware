# $Id: Makefile.win,v 1.11 2008/05/07 19:06:37 carregal Exp $

T= lfs

include ./config.win

SRCS= src/$T.c
OBJS= src/$T.obj

lib: src/lfs.dll

src/$T.c:
	$(CC) /c /Fo$@ $(CFLAGS) $<

src/$T.obj:
	$(CC) /c /Fo$@ $(CFLAGS) src/$T.c

src/lfs.dll: $(OBJS)
	$(LN) /dll /out:src/$T.dll /def:src/$T.def $(OBJS) $(LUA_LIB) $(LIB)
	#IF EXIST src\lfs.dll.manifest mt -manifest src\lfs.dll.manifest -outputresource:src\lfs.dll;2

install: src/lfs.dll
	mkdir -p "$(LUA_LIBDIR)"
	cp src/lfs.dll "$(LUA_LIBDIR)/"

clean:
	rm src/lfs.dll $(OBJS) src/$T.lib src/$T.exp
	rm src/lfs.dll.manifest

