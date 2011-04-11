noinst_LTLIBRARIES = libumms-ginterfaces.la
libexec_PROGRAMS = umms-server

umms_server_CFLAGS = $(UMMS_CFLAGS)

umms_server_LDADD = libumms-ginterfaces.la $(UMMS_CFLAGS)
umms_server_SOURCES = umms-common.h umms-server-main.c \
                      meego-media-player.c meego-media-player.h \
					  meego-media-player-gstreamer.c meego-media-player-gstreamer.h \
				      meego-media-player-control.c meego-media-player-control.h \
					  engine-gst.c engine-gst.h \
					  umms-object-manager-glue.h umms-object-manager.c umms-object-manager.h

%.c: %.list Makefile.am
	$(AM_V_GEN)glib-genmarshal --body --prefix=umms_marshal $< > $@

%.h: %.list Makefile.am
	$(AM_V_GEN)glib-genmarshal --header --prefix=umms_marshal $< > $@

MARSHALS_SOURCES = \
	umms-marshals.c umms-marshals.h

MARSHALS_LISTS = umms-marshals.list

DBUS_SPECS = umms.xml

%-ginterface.h %-ginterface.c: %.xml Makefile.am
	$(AM_V_GEN)python ./tools/glib-ginterface-gen.py --include='"umms-marshals.h"' --filename=$(basename $@) $< Umms_

GINTERFACE_SOURCES = \
	$(DBUS_SPECS:.xml=-ginterface.h) \
	$(DBUS_SPECS:.xml=-ginterface.c)

libumms_ginterfaces_la_SOURCES = $(GINTERFACE_SOURCES) $(MARSHALS_SOURCES)
libumms_ginterfaces_la_LIBADD = $(UMMS_LIBS)
libumms_ginterfaces_la_CFLAGS = $(UMMS_CFLAGS)

BUILT_SOURCES = $(GINTERFACE_SOURCES) $(MARSHALS_SOURCES)

%.service: %.service.in ../config.log
	$(AM_V_GEN)sed -e "s|\@dir\@|$(libexecdir)|" $< > $@

servicedir = $(datadir)/dbus-1/services/
service_in_files = umms-server.service.in
service_DATA = umms-server.service

CLEANFILES = $(BUILT_SOURCES) $(service_DATA)
EXTRA_DIST = $(DBUS_SPECS) $(MARSHALS_LISTS) $(service_in_files)
