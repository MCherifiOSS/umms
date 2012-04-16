INCLUDES = \
	-I$(top_srcdir)/libummsclient \
	$(UMMS_SAMPLE_UI_CFLAGS)

LDADD = \
	$(top_builddir)/libummsclient/libummsclient-@UMMS_MAJORMINOR@.la \
	$(UMMS_SAMPLE_UI_LIBS)

noinst_PROGRAMS = gtk-UMMS-test

gtk_UMMS_test_SOURCES = main.c  gtk-backend-gst.c  gtk-backend.h  umms-gtk-player.c  umms-gtk-player.h  umms-gtk-ui.c  umms-gtk-ui.h  gtk-backend-umms.c
