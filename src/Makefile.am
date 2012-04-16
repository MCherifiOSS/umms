libexec_PROGRAMS = umms-server

umms_server_CFLAGS = $(UMMS_SERVER_CFLAGS)

umms_server_LDADD = $(UMMS_SERVER_LIBS)

GLUE = \
       ./glue/umms-object-manager-glue.h \
       ./glue/umms-media-player-glue.h \
       ./glue/umms-playing-content-metadata-viewer-glue.h \
       ./glue/umms-audio-manager-glue.h \
       ./glue/umms-video-output-glue.h

MARSHALS = \
    umms-marshals.c umms-marshals.h

VERSION_HEADER = umms-version.h

GENERATED_SOURCE = $(GLUE) $(MARSHALS) $(VERSION_HEADER)

# BUILT_SOURCES are built on make all/check/install before all other targets
BUILT_SOURCES = $(GENERATED_SOURCE)

umms_server_SOURCES =  umms-server.h \
		       umms-types.h \
		       umms-debug.h \
		       umms-error.h \
		       umms-error.c \
		       umms-plugin.h \
		       umms-plugin.c \
		       umms-utils.h \
		       umms-utils.c \
		       umms-server-main.c \
		       umms-media-player.c \
		       umms-media-player.h \
		       umms-player-backend.c \
		       umms-player-backend.h \
		       umms-backend-factory.c \
		       umms-backend-factory.h \
		       umms-object-manager.c \
		       umms-object-manager.h \
		       umms-audio-manager.c \
		       umms-audio-manager.h \
		       umms-audio-manager-backend.c \
		       umms-audio-manager-backend.h \
		       umms-video-output.c \
		       umms-video-output.h \
		       umms-video-output-backend.c \
		       umms-video-output-backend.h \
		       umms-resource-manager.c \
		       umms-resource-manager.h \
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
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=umms_media_player --mode=glib-server ../spec/umms-media-player.xml > ./glue/umms-media-player-glue.h
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=umms_playing_content_metadata_viewer --mode=glib-server ../spec/umms-playing-content-metadata-viewer.xml > ./glue/umms-playing-content-metadata-viewer-glue.h
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=umms_audio_manager --mode=glib-server ../spec/umms-audio-manager.xml > ./glue/umms-audio-manager-glue.h
	$(LIBTOOL) --mode=execute dbus-binding-tool --prefix=umms_video_output --mode=glib-server ../spec/umms-video-output.xml > ./glue/umms-video-output-glue.h

#framework library for the plugin development
lib_LTLIBRARIES=libumms-@UMMS_MAJOR_VERSION@.@UMMS_MINOR_VERSION@.la

libumms_@UMMS_MAJOR_VERSION@_@UMMS_MINOR_VERSION@_la_SOURCES = \
		     umms-error.c \
		     umms-utils.c \
		     umms-marshals.c \
		     umms-plugin.c \
		     umms-resource-manager.c \
		     umms-player-backend.c \
		     umms-video-output-backend.c \
		     umms-audio-manager-backend.c

libumms_@UMMS_MAJOR_VERSION@_@UMMS_MINOR_VERSION@_includedir = $(includedir)/umms-@UMMS_MAJOR_VERSION@.@UMMS_MINOR_VERSION@

libumms_@UMMS_MAJOR_VERSION@_@UMMS_MINOR_VERSION@_include_HEADERS = umms.h \
													umms-version.h \
													umms-debug.h \
													umms-types.h \
													umms-error.h \
													umms-utils.h \
													umms-marshals.h \
													umms-plugin.h \
													umms-resource-manager.h \
													umms-player-backend.h\
													umms-video-output-backend.h \
													umms-audio-manager-backend.h

libumms_@UMMS_MAJOR_VERSION@_@UMMS_MINOR_VERSION@_la_CFLAGS = $(UMMS_LIB_CFLAGS)
libumms_@UMMS_MAJOR_VERSION@_@UMMS_MINOR_VERSION@_la_LIBADD = $(UMMS_LIB_LIBS)

%.service: %.service.in ../config.log
	$(AM_V_GEN)sed -e "s|\@dir\@|$(libexecdir)|" $< > $@

servicedir = $(datadir)/dbus-1/system-services/

service_in_files = com.UMMS.service.in
service_DATA = com.UMMS.service

confdir = /etc/dbus-1/system.d/
dist_conf_DATA = com.UMMS.conf

# do not put files in the distribution that are generated
#nodist_libumms_@UMMS_MAJOR_VERSION@_@UMMS_MINOR_VERSION@_la_SOURCES = $(GENERATED_SOURCE)

CLEANFILES = $(GENERATED_SOURCE) $(service_DATA) 
EXTRA_DIST = umms-marshals.list $(service_in_files)
