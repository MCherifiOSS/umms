libexec_PROGRAMS = umms-server

umms_server_CFLAGS = $(UMMS_CFLAGS)

umms_server_LDADD = $(UMMS_LIBS) -lgstapp-0.10
umms_server_includedir = $(includedir)/umms
umms_server_include_HEADERS = \
															umms-common.h \
															umms-error.h \
															umms-debug.h \
															umms-video-output.h


GLUE = \
              ./glue/umms-object-manager-glue.h \
              ./glue/umms-media-player-glue.h \
              ./glue/umms-playing-content-metadata-viewer-glue.h \
              ./glue/umms-audio-manager-glue.h


MARSHALS = \
    umms-marshals.c umms-marshals.h

GENERATED_SOURCE = $(GLUE) $(MARSHALS)
BUILT_SOURCES = $(GENERATED_SOURCE)

umms_server_SOURCES = umms-common.h \
                      umms-debug.h \
                      umms-error.h \
                      umms-error.c \
                      umms-server-main.c \
                      media-player.c \
                      media-player.h \
                      player-control-factory.c \
                      player-control-factory.h \
                      media-player-control.c \
                      media-player-control.h \
                      player-control-base.c \
                      player-control-base.h \
                      player-control-tv.c \
                      player-control-tv.h \
                      player-control-generic.c \
                      player-control-generic.h \
                      dvb-player-control-base.c \
                      dvb-player-control-base.h \
                      dvb-player-control-tv.c \
                      dvb-player-control-tv.h \
                      dvb-player-control-generic.c \
                      dvb-player-control-generic.h \
                      umms-object-manager.c \
                      umms-object-manager.h \
                      param-table.c \
                      param-table.h \
                      umms-resource-manager.c \
                      umms-resource-manager.h \
                      umms-playing-content-metadata-viewer.c \
                      umms-playing-content-metadata-viewer.h \
                      umms-audio-manager.c \
                      umms-audio-manager.h \
                      audio-manager-interface.c \
                      audio-manager-interface.h \
                      audio-manager-engine.c \
                      audio-manager-engine.h \
                      umms-video-output.c \
                      umms-video-output.h \
                      $(GENERATED_SOURCE)

%.c: %.list Makefile.am
	$(AM_V_GEN)glib-genmarshal --body --prefix=umms_marshal $< > $@

%.h: %.list Makefile.am
	$(AM_V_GEN)glib-genmarshal --header --prefix=umms_marshal $< > $@

./glue/%-glue.h: ../spec/%.xml Makefile.am
	$(AM_V_GEN)mkdir -p glue
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=umms_object_manager --mode=glib-server ../spec/umms-object-manager.xml >  ./glue/umms-object-manager-glue.h
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=media_player --mode=glib-server ../spec/umms-media-player.xml > ./glue/umms-media-player-glue.h
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=umms_playing_content_metadata_viewer --mode=glib-server ../spec/umms-playing-content-metadata-viewer.xml > ./glue/umms-playing-content-metadata-viewer-glue.h
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=umms_audio_manager --mode=glib-server ../spec/umms-audio-manager.xml > ./glue/umms-audio-manager-glue.h
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=umms_video_output --mode=glib-server ../spec/umms-video-output.xml > ./glue/umms-video-output-glue.h


MARSHALS_LISTS = umms-marshals.list




%.service: %.service.in ../config.log
	$(AM_V_GEN)sed -e "s|\@dir\@|$(libexecdir)|" $< > $@

servicedir = $(datadir)/dbus-1/system-services/

service_in_files = com.UMMS.service.in
service_DATA = com.UMMS.service

confdir = /etc/dbus-1/system.d/
dist_conf_DATA = com.UMMS.conf

CLEANFILES = $(GENERATED_SOURCE) $(service_DATA) 
EXTRA_DIST = $(MARSHALS_LISTS) $(service_in_files)
