#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])

dnl The umms version number 
m4_define(umms_major_version, 0)
m4_define(umms_minor_version, 1)
m4_define(umms_micro_version, 0)

m4_define(umms_version, umms_major_version.umms_minor_version.umms_micro_version)
m4_define(umms_majorminor, umms_major_version.umms_minor_version)

AC_INIT(umms, umms_version, [zhiwen.wu@intel.com])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(umms, umms_version)

AC_SUBST(UMMS_MAJOR_VERSION, umms_major_version)
AC_SUBST(UMMS_MINOR_VERSION, umms_minor_version)
AC_SUBST(UMMS_MICRO_VERSION, umms_micro_version)
AC_SUBST(UMMS_MAJORMINOR, umms_majorminor)

LT_PREREQ([2.2.6])
LT_INIT()

# Checks for programs.
AC_PROG_CC

AM_PROG_CC_C_O

# Checks for libraries.

PKG_CHECK_MODULES(UMMS_SERVER,       \
                  glib-2.0 >= 2.24   \
                  gthread-2.0 \
                  gmodule-2.0 >= 2.24\
                  dbus-glib-1)

PKG_CHECK_MODULES(UMMS_SAMPLE, glib-2.0 >= 2.24 \
                               dbus-glib-1)

PKG_CHECK_MODULES(UMMS_SAMPLE_UI, 	        \
                  glib-2.0 >= 2.24 	        \
                  gstreamer-interfaces-0.10 \
                  x11                       \
                  gtk+-2.0                  \
                  dbus-glib-1)

PKG_CHECK_MODULES(UMMSCLIENT_LIB, \
		  dbus-glib-1)

PKG_CHECK_MODULES(UMMS_LIB, glib-2.0 >= 2.24)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Checks for library functions.

AC_CONFIG_FILES([Makefile
								 umms-${UMMS_MAJOR_VERSION}.${UMMS_MINOR_VERSION}.pc:umms.pc.in
								 ummsclient-${UMMS_MAJOR_VERSION}.${UMMS_MINOR_VERSION}.pc:ummsclient.pc.in
								 test/Makefile
								 test/ui/Makefile
                 libummsclient/Makefile
                 spec/Makefile
                 scripts/Makefile
                 src/Makefile
								 src/umms-version.h])
AC_OUTPUT
