AC_PREREQ(2.57)
AC_INIT([nautilus-share],[0.7.3])
AC_CONFIG_SRCDIR([src/nautilus-share.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign 1.9 -Wall])

AC_PROG_INTLTOOL([0.29])

NAUTILUS_REQUIRED=2.10.0
GLIB_REQUIRED=2.4.0

AC_PROG_CC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(NAUTILUS, libnautilus-extension >= $NAUTILUS_REQUIRED)
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)

dnl ========================================================
dnl Get nautilus extensionsdir
dnl ========================================================

nautilus_extensiondir=`$PKG_CONFIG --variable=extensiondir libnautilus-extension`
AC_SUBST([nautilus_extensiondir])

dnl ========================================================
dnl Set interfacesdir
dnl ========================================================
AC_SUBST([interfacesdir], ['${pkgdatadir}/interfaces'])

dnl ========================================================
dnl Translations
dnl ========================================================

ALL_LINGUAS="ar as ast bn_IN ca ca@valencia cs da de el en_GB eo es et de dz eu fi fr gl gu he hu id it kn ko lt lv ml mr nb oc pa pl pt pt_BR ro ru sk sl sv ta te uk vi zh_CN zh_HK zh_TW"
GETTEXT_PACKAGE=nautilus-share
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [gettext package name])
AM_GLIB_GNU_GETTEXT
AM_GLIB_DEFINE_LOCALEDIR(NAUTILUS_SHARE_LOCALEDIR)

dnl ========================================================

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  interfaces/Makefile
  po/Makefile.in
])
AC_OUTPUT

