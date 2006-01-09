libnautilus_share_la_CFLAGS =				\
	-Wall						\
	-DG_LOG_DOMAIN=\"Nautilus-Share\"		\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	-DDATADIR=\"$(datadir)\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)                                  \
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(NAUTILUS_CFLAGS)				\
	$(DBUS_CFLAGS)					\
	$(LIBGLADE_CFLAGS)				\
	$(GLIB_CFLAGS)

smbshared_CFLAGS =					\
	-Wall						\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	-DDATADIR=\"$(datadir)\"			\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)                                  \
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(DBUS_CFLAGS)					\
	$(LIBDAEMON_CFLAGS)				\
	$(GLIB_CFLAGS)

# FIXME: get this from the .pc file
nautilus_extensiondir=$(libdir)/nautilus/extensions-1.0

nautilus_extension_LTLIBRARIES=libnautilus-share.la

bin_PROGRAMS = smbshared

libnautilus_share_la_SOURCES =  \
	smbparser-dbus-client.c		\
	smbparser-dbus-client.h		\
	smbparser-dbus.h		\
	nautilus-share.c        \
	nautilus-share.h           

smbshared_SOURCES = \
	smbparser.c		\
	smbparser.h		\
	smbparser-dbus.h		\
	smbparser-dbus-server.c  \
	smbparser-dbus-server.h \
	smbshared.c

libnautilus_share_la_LDFLAGS = -module -avoid-version
libnautilus_share_la_LIBADD  = $(GLIB_LIBS) $(NAUTILUS_LIBS) $(DBUS_LIBS) $(LIBGLADE_LIBS)

smbshared_LDADD  = $(GLIB_LIBS) $(DBUS_LIBS) $(LIBDAEMON_LIBS)

