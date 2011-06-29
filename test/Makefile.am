noinst_PROGRAMS = client-test-gobject engine-test
client_test_gobject_CFLAGS = $(UMMSCLIENT_CFLAGS)
client_test_gobject_LDFLAGS = $(UMMSCLIENT_LIBS)
client_test_gobject_LDADD = ../libummsclient/libummsclient.a
client_test_gobject_SOURCES = test-common.c test-common.h client-test-gobject.c \
							  ../src/umms-marshals.c \
							  ../src/umms-marshals.h

engine_test_CFLAGS = $(UMMS_CFLAGS)
engine_test_LDADD = $(UMMS_LIBS)
engine_test_SOURCES = test-common.c test-common.h ../src/meego-media-player-control.c ../src/meego-media-player-control.h \
					  ../src/engine-gst.c ../src/engine-gst.h \
					  ../src/umms-marshals.c ../src/umms-marshals.h \
					  engine-test.c

EXTRA_DIST = client-test.py
