
UMMS_PC=umms-@UMMS_MAJOR_VERSION@.@UMMS_MINOR_VERSION@.pc
UMMSCLIENT_PC=ummsclient-@UMMS_MAJOR_VERSION@.@UMMS_MINOR_VERSION@.pc

SUBDIRS=src spec libummsclient test test/ui scripts
ACLOCAL_AMFLAGS = -I m4


DISTCLEANFILES = 		\
	$(UMMS_PC) \
	$(UMMSCLIENT_PC)

EXTRA_DIST = umms.pc.in ummsclient.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(UMMS_PC) $(UMMSCLIENT_PC)

#install template configure file
confdir = /etc/
dist_conf_DATA = umms.conf
