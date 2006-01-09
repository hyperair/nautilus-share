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

#ifndef NAUTILUS_SHARE_H
#define NAUTILUS_SHARE_H

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <glib-object.h>
#include <dbus/dbus-glib-lowlevel.h>

G_BEGIN_DECLS

/* Declarations for the Share extension object.  This object will be
 * instantiated by nautilus.  It implements the GInterfaces
 * exported by libnautilus. */


#define NAUTILUS_TYPE_SHARE  (nautilus_share_get_type ())

typedef struct _NautilusShare      NautilusShare;
typedef struct _NautilusShareClass NautilusShareClass;

struct _NautilusShare {
	GObject parent_slot;
};

struct _NautilusShareClass {
	GObjectClass parent_slot;

	/* No extra class members */
};


typedef struct _NautilusShareData      NautilusShareData;

struct _NautilusShareData {
  gchar		*fullpath;
  gchar		*section;
  NautilusFileInfo *fileinfo;
};

GType nautilus_share_get_type      (void);
void  nautilus_share_register_type (GTypeModule *module);

G_END_DECLS

typedef enum {
  NAUTILUS_SHARE_NOT_SHARED,
  NAUTILUS_SHARE_SHARED_RO,
  NAUTILUS_SHARE_SHARED_RW
} NautilusShareStatus;

static NautilusShareStatus file_get_share_status (gchar *fullpath);
#endif
 
