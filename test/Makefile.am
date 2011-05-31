bin_PROGRAMS = test-client-gobject
test_client_gobject_CFLAGS = $(UMMSCLIENT_CFLAGS)
test_client_gobject_LDFLAGS = $(UMMSCLIENT_LIBS)
test_client_gobject_LDADD = ../libummsclient/libummsclient.a
test_client_gobject_SOURCES = test-client-gobject.c \
							  ../src/umms-marshals.c \
							  ../src/umms-marshals.h

EXTRA_DIST = test-client.py
