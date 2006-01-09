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

#ifndef _SMBPARSER_H_
#define _SMBPARSER_H_

#include <glib.h>

#define SMB_CONF "/etc/samba/smbshared.conf"

/*----------------------------------------------------------------------*/
typedef struct {
  gchar *sharename;
  gchar *username;
  gchar *sharepath;
  GSList *directives;
} Share;

typedef struct {
  gchar *key;
  gchar *value;
} Directive;

/*----------------------------------------------------------------------*/
/* in daemon*/
GSList *smbparser_load( GSList *smbparser, const gchar *filename);
void smbparser_write(GSList *smbparser, const gchar *filename);
void free_shares(GSList *smbparser);

GSList *smbparser_remove_key(GSList *smbparser, gchar *section, gchar *key);
GSList *smbparser_set_key(GSList *smbparser, gchar *section, gchar *key, gchar *value);
GSList *smbparser_rename_section(GSList *smbparser, gchar *section, gchar *newname);
GSList *smbparser_remove_section(GSList *smbparser, gchar *section);
GSList *smbparser_add_section(GSList *smbparser, gchar *section, gchar *username);
gchar *smbparser_get_section_owner(GSList *smbparser, gchar *section);
gchar *smbparser_get_key(GSList *smbparser, gchar *section, gchar *key);
gchar *smbparser_get_sharename(GSList *smbparser, gchar *path);
gboolean smbparser_sharename_is_writable(GSList *smbparser, gchar *path);
GSList *find_sharename(GSList *shares, gchar *sharename);
void dump(GSList *shares);

#endif
