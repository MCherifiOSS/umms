libexec_PROGRAMS = umms-server

umms_server_CFLAGS = $(UMMS_CFLAGS)

umms_server_LDADD = $(UMMS_LIBS)
umms_server_SOURCES = umms-common.h umms-server-main.c \
                      meego-media-player.c meego-media-player.h \
					  meego-media-player-gstreamer.c meego-media-player-gstreamer.h \
				      meego-media-player-control.c meego-media-player-control.h \
					  engine-gst.c engine-gst.h \
					  umms-object-manager.c umms-object-manager.h \
					  umms-marshals.c umms-marshals.h umms-debug.h \
					  ./glue/umms-object-manager-glue.h \
					  ./glue/umms-media-player-glue.h


%.c: %.list Makefile.am
	$(AM_V_GEN)glib-genmarshal --body --prefix=umms_marshal $< > $@

%.h: %.list Makefile.am
	$(AM_V_GEN)glib-genmarshal --header --prefix=umms_marshal $< > $@

./glue/%-glue.h: ../spec/%.xml Makefile.am
	$(AM_V_GEN)mkdir -p glue
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=umms_object_manager --mode=glib-server ../spec/umms-object-manager.xml >  ./glue/umms-object-manager-glue.h
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=meego_media_player --mode=glib-server ../spec/umms-media-player.xml > ./glue/umms-media-player-glue.h

MARSHALS = \
	umms-marshals.c umms-marshals.h

MARSHALS_LISTS = umms-marshals.list


GLUE = \
			  ./glue/umms-object-manager-glue.h \
			  ./glue/umms-media-player-glue.h 

BUILT_SOURCES = $(GLUE) $(MARSHALS)

%.service: %.service.in ../config.log
	$(AM_V_GEN)sed -e "s|\@dir\@|$(libexecdir)|" $< > $@

servicedir = $(datadir)/dbus-1/services/
service_in_files = umms-server.service.in
service_DATA = umms-server.service

CLEANFILES = $(BUILT_SOURCES) $(service_DATA)
EXTRA_DIST = $(MARSHALS_LISTS) $(service_in_files)
