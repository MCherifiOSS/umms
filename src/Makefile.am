libexec_PROGRAMS = umms-server

umms_server_CFLAGS = $(UMMS_CFLAGS)

umms_server_LDADD = $(UMMS_LIBS)


GLUE = \
			  ./glue/umms-object-manager-glue.h \
			  ./glue/umms-media-player-glue.h \
			  ./glue/umms-playing-content-metadata-viewer-glue.h


MARSHALS = \
	umms-marshals.c umms-marshals.h

GENERATED_SOURCE = $(GLUE) $(MARSHALS)
BUILT_SOURCES = $(GENERATED_SOURCE)

umms_server_SOURCES = umms-common.h umms-server-main.c \
					  umms-error.h umms-error.c \
                      meego-media-player.c meego-media-player.h \
					  meego-media-player-gstreamer.c meego-media-player-gstreamer.h \
				      meego-media-player-control.c meego-media-player-control.h \
					  engine-gst.c engine-gst.h \
					  umms-object-manager.c umms-object-manager.h \
					  umms-debug.h \
						param-table.c param-table.h \
						umms-resource-manager.c umms-resource-manager.h \
                                                umms-playing-content-metadata-viewer.c \
                                                umms-playing-content-metadata-viewer.h \
					        $(GENERATED_SOURCE)


%.c: %.list Makefile.am
	$(AM_V_GEN)glib-genmarshal --body --prefix=umms_marshal $< > $@

%.h: %.list Makefile.am
	$(AM_V_GEN)glib-genmarshal --header --prefix=umms_marshal $< > $@

./glue/%-glue.h: ../spec/%.xml Makefile.am
	$(AM_V_GEN)mkdir -p glue
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=umms_object_manager --mode=glib-server ../spec/umms-object-manager.xml >  ./glue/umms-object-manager-glue.h
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=meego_media_player --mode=glib-server ../spec/umms-media-player.xml > ./glue/umms-media-player-glue.h
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=umms_playing_content_metadata_viewer --mode=glib-server ../spec/umms-playing-content-metadata-viewer.xml > ./glue/umms-playing-content-metadata-viewer-glue.h


MARSHALS_LISTS = umms-marshals.list




%.service: %.service.in ../config.log
	$(AM_V_GEN)sed -e "s|\@dir\@|$(libexecdir)|" $< > $@

servicedir = $(datadir)/dbus-1/system-services/

service_in_files = com.meego.UMMS.service.in
service_DATA = com.meego.UMMS.service

confdir = /etc/dbus-1/system.d/
dist_conf_DATA = com.meego.UMMS.conf

CLEANFILES = $(GENERATED_SOURCE) $(service_DATA) 
EXTRA_DIST = $(MARSHALS_LISTS) $(service_in_files)
