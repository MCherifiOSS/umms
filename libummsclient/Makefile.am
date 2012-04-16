lib_LTLIBRARIES=libummsclient-@UMMS_MAJOR_VERSION@.@UMMS_MINOR_VERSION@.la
libummsclient_@UMMS_MAJOR_VERSION@_@UMMS_MINOR_VERSION@_la_SOURCES = \
	umms-client-object.c \
	umms-client-object.h \
	../src/umms-marshals.c \
	../src/umms-marshals.h

libummsclient_@UMMS_MAJOR_VERSION@_@UMMS_MINOR_VERSION@_includedir = $(includedir)/ummsclient-@UMMS_MAJOR_VERSION@.@UMMS_MINOR_VERSION@

libummsclient_@UMMS_MAJOR_VERSION@_@UMMS_MINOR_VERSION@_include_HEADERS = umms-client-object.h ../src/umms-marshals.h
	                                                            
libummsclient_@UMMS_MAJOR_VERSION@_@UMMS_MINOR_VERSION@_la_CFLAGS = $(UMMSCLIENT_LIB_CFLAGS)
libummsclient_@UMMS_MAJOR_VERSION@_@UMMS_MINOR_VERSION@_la_LIBADD = $(UMMSCLIENT_LIB_LIBS)

EXTRA_DIST = libummsclient.py
