all:
	$(CC) mdhrtsp-streamer.c -o ./mdhrtsp-streamer.o -pthread -D_REENTRANT -I$(PREFIX)/usr/include/glib-2.0 -I$(PREFIX)/usr/lib/glib-2.0/include -I$(PREFIX)/usr/include/gstreamer-0.10 -I$(PREFIX)/usr/include  -pthread -L$(PREFIX)/usr/lib -lgstrtspserver-0.10 -lgstbase-0.10 -lgstreamer-0.10 -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0 -lavahi-common -lavahi-core
clean:
	$(shell rm *.o)

distclean:
	$(shell rm *.o)
