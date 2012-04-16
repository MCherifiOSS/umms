INCLUDES = \
	-I$(top_srcdir)/libummsclient \
	$(UMMS_SAMPLE_CFLAGS)

LDADD = \
	$(top_builddir)/libummsclient/libummsclient-@UMMS_MAJORMINOR@.la \
	$(UMMS_SAMPLE_LIBS)

noinst_PROGRAMS = client-test-gobject
client_test_gobject_SOURCES = test-common.c test-common.h client-test-gobject.c

EXTRA_DIST = client-test.py
