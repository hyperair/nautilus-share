/* nautilus-share -- Nautilus File Sharing Extension
 *
 * Sebastien Estienne <sebest@ethium.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * (C) Copyright 2005 Ethium, Inc.
 */

#ifndef _SMBPARSER_DBUS_CLIENT_H_
#define _SMBPARSER_DBUS_CLIENT_H_

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

gboolean smbparser_dbus_dump(DBusConnection *bus);
gboolean smbparser_dbus_write(DBusConnection *bus);

unsigned int smbparser_dbus_section_used(DBusConnection *bus, gchar *section);
gchar * smbparser_dbus_share_get_key(DBusConnection *bus, gchar *path, gchar *key);
gchar * smbparser_dbus_share_get_name(DBusConnection *bus, gchar *path);
gboolean  smbparser_dbus_share_is_writable(DBusConnection *bus, gchar *path);
gboolean smbparser_dbus_share_remove_key(DBusConnection *bus, gchar *path, gchar *key);
gboolean smbparser_dbus_share_set_key(DBusConnection *bus, gchar *path, gchar *key, gchar *value);
unsigned int smbparser_dbus_share_rename(DBusConnection *bus, gchar *path, gchar *newname);
gboolean smbparser_dbus_share_remove(DBusConnection *bus, gchar *path);
unsigned int smbparser_dbus_share_add(DBusConnection *bus, gchar *path, gchar *name);
GSList * smbparser_dbus_get_sharepaths(DBusConnection *bus);

#endif
