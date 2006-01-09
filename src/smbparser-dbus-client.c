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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <string.h>

#include "smbparser-dbus.h"
#include "smbparser-dbus-client.h"

/******************************************************************************/
gboolean 
smbparser_dbus_dump(DBusConnection *bus)
{
  DBusMessage *message;

  message = dbus_message_new_signal (SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "Dump");
  dbus_connection_send (bus, message, NULL);
  dbus_message_unref (message);
  return TRUE;
}

/******************************************************************************/
gboolean 
smbparser_dbus_write(DBusConnection *bus)
{
  DBusMessage *message;

  message = dbus_message_new_signal (SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "WriteConf");
  dbus_connection_send (bus, message, NULL);
  dbus_message_unref (message);
  return TRUE;
}

/******************************************************************************/
gboolean 
smbparser_dbus_remove_key(DBusConnection *bus, gchar *section, gchar *key)
{
  DBusMessage *message;

  if(!section || !key)
    return FALSE;
  message = dbus_message_new_signal (SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "RemoveKey");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, section,
                            DBUS_TYPE_STRING, key,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &section,
                            DBUS_TYPE_STRING, &key,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);
  dbus_connection_send (bus, message, NULL);
  dbus_message_unref (message);
  return TRUE;
}

/******************************************************************************/
gboolean 
smbparser_dbus_set_key(DBusConnection *bus, gchar *path, gchar *key, gchar *value)
{
  DBusMessage *message;

  if (!path || !key || !value)
    return FALSE;
  message = dbus_message_new_signal (SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "SetKey");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, path,
                            DBUS_TYPE_STRING, key,
                            DBUS_TYPE_STRING, value,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &path,
                            DBUS_TYPE_STRING, &key,
                            DBUS_TYPE_STRING, &value,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);
  dbus_connection_send (bus, message, NULL);
  dbus_message_unref (message);
  return TRUE;
}

/******************************************************************************/
gboolean 
smbparser_dbus_share_remove_key(DBusConnection *bus, gchar *path, gchar *key)
{
  DBusMessage *message;

  if(!path || !key)
    return FALSE;
  message = dbus_message_new_signal (SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "ShareRemoveKey");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, path,
                            DBUS_TYPE_STRING, key,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &path,
                            DBUS_TYPE_STRING, &key,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);
  dbus_connection_send (bus, message, NULL);
  dbus_message_unref (message);
  return TRUE;
}

/******************************************************************************/
gboolean 
smbparser_dbus_share_set_key(DBusConnection *bus, gchar *path, gchar *key, gchar *value)
{
  DBusMessage *message;

  if (!path || !key || !value)
    return FALSE;
  message = dbus_message_new_signal (SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "ShareSetKey");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, path,
                            DBUS_TYPE_STRING, key,
                            DBUS_TYPE_STRING, value,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &path,
                            DBUS_TYPE_STRING, &key,
                            DBUS_TYPE_STRING, &value,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);
  dbus_connection_send (bus, message, NULL);
  dbus_message_unref (message);
  return TRUE;
}

/******************************************************************************/
gboolean 
smbparser_dbus_remove_section(DBusConnection *bus, gchar *section)
{
  DBusMessage *message;

  if(!section)
    return FALSE;
  message = dbus_message_new_signal (SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "RemoveSection");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, section,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &section,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);
  dbus_connection_send (bus, message, NULL);
  dbus_message_unref (message);
  return TRUE;
}

/******************************************************************************/
gboolean 
smbparser_dbus_share_remove(DBusConnection *bus, gchar *path)
{
  DBusMessage *message;

  if(!path)
    return FALSE;
  message = dbus_message_new_signal (SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "ShareRemove");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, path,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &path,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);
  dbus_connection_send (bus, message, NULL);
  dbus_message_unref (message);
  return TRUE;
}

/******************************************************************************/
unsigned int
smbparser_dbus_rename_section(DBusConnection *bus, gchar *section, gchar *newname)
{
  DBusMessage *message;
  DBusMessage	*reply;
  DBusError	error;
  DBusError	error_args;
  unsigned int	res;

  if (!section || !newname)
    return FALSE;
  message = dbus_message_new_method_call (SMBPARSER_DBUS_SRV, SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "RenameSection");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, section,
                            DBUS_TYPE_STRING, newname,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &section,
                            DBUS_TYPE_STRING, &newname,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);

  dbus_error_init (&error);
  if((reply = dbus_connection_send_with_reply_and_block (bus, message, -1, &error)))
    {
      dbus_error_init (&error_args);
      if(!dbus_message_get_args (reply, &error_args, DBUS_TYPE_UINT32, &res, DBUS_TYPE_INVALID))
	g_print("RenameSection error in dbus_message_get_args: %s\n",error_args.message);
      dbus_message_unref (reply);
      dbus_error_free (&error_args);
    }
  else
    g_print("RenameSection error in reply: %s\n",error.message);
  dbus_error_free (&error);
  dbus_message_unref (message);
  return res;
}


/******************************************************************************/
unsigned int
smbparser_dbus_share_rename(DBusConnection *bus, gchar *path, gchar *newname)
{
  DBusMessage *message;
  DBusMessage	*reply;
  DBusError	error;
  DBusError	error_args;
  unsigned int	res;

  if (!path || !newname)
    return FALSE;
  message = dbus_message_new_method_call (SMBPARSER_DBUS_SRV, SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "ShareRename");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, path,
                            DBUS_TYPE_STRING, newname,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &path,
                            DBUS_TYPE_STRING, &newname,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);

  dbus_error_init (&error);
  if((reply = dbus_connection_send_with_reply_and_block (bus, message, -1, &error)))
    {
      dbus_error_init (&error_args);
      if(!dbus_message_get_args (reply, &error_args, DBUS_TYPE_UINT32, &res, DBUS_TYPE_INVALID))
	g_print("ShareRename error in dbus_message_get_args: %s\n",error_args.message);
      dbus_message_unref (reply);
      dbus_error_free (&error_args);
    }
  else
    g_print("ShareRename error in reply: %s\n",error.message);
  dbus_error_free (&error);
  dbus_message_unref (message);
  return res;
}


/******************************************************************************/
GSList *
smbparser_dbus_get_sharepaths(DBusConnection *bus)
{
  DBusMessage	*message;
  DBusMessage	*reply;
  DBusError	error;
  DBusError	error_args;
  char		**sharenames = NULL;
  DBusMessageIter iter;
  char		*tmp;
  GSList	*sharepaths = NULL;

  message = dbus_message_new_method_call (SMBPARSER_DBUS_SRV, SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "GetSharepaths");
  dbus_error_init (&error);
  if((reply = dbus_connection_send_with_reply_and_block (bus, message, -1, &error)))
    {
      dbus_error_init (&error_args);
      if(!dbus_message_get_args (reply, &error_args, DBUS_TYPE_STRING, &sharenames, DBUS_TYPE_INVALID))
	g_print("GetSharepaths error in dbus_message_get_args: %s\n",error_args.message);

      dbus_message_iter_init( reply, &iter );
      
#ifndef DBUS_USE_NEW_API
      while((tmp = dbus_message_iter_get_string(&iter)) && strlen(tmp))
#else /* DBUS_USE_NEW_API */
      while((dbus_message_iter_get_basic(&iter, &tmp), strlen(tmp)))
#endif /* DBUS_USE_NEW_API */
	{
	  sharepaths = g_slist_append(sharepaths,tmp);
	  dbus_message_iter_next(&iter);
	}

      dbus_message_unref (reply);
      dbus_error_free (&error_args);
    }
  else
    g_print("GetSharepaths error in reply: %s\n",error.message);
  dbus_error_free (&error);
  dbus_message_unref (message);
  return sharepaths;
}

/******************************************************************************/
unsigned int
smbparser_dbus_share_add(DBusConnection *bus, gchar *path, gchar *name)
{
  DBusMessage	*message;
  DBusMessage	*reply;
  DBusError	error;
  DBusError	error_args;
  unsigned int	res;

  if(!path || !name)
    return FALSE;
  message = dbus_message_new_method_call (SMBPARSER_DBUS_SRV, SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "ShareAdd");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, path,
                            DBUS_TYPE_STRING, name,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &path,
                            DBUS_TYPE_STRING, &name,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);
  dbus_error_init (&error);
  if((reply = dbus_connection_send_with_reply_and_block (bus, message, -1, &error)))
    {
      dbus_error_init (&error_args);
      if(!dbus_message_get_args (reply, &error_args, DBUS_TYPE_UINT32, &res, DBUS_TYPE_INVALID))
	g_print("ShareAdd error in dbus_message_get_args: %s\n",error_args.message);
      dbus_message_unref (reply);
      dbus_error_free (&error_args);
    }
  else
    g_print("ShareAdd error in reply: %s\n",error.message);
  dbus_error_free (&error);
  dbus_message_unref (message);
  return res;
}
/******************************************************************************/
unsigned int
smbparser_dbus_section_used(DBusConnection *bus, gchar *section)
{
  DBusMessage	*message;
  DBusMessage	*reply;
  DBusError	error;
  DBusError	error_args;
  unsigned int	res;

  if(!section)
    return FALSE;
  message = dbus_message_new_method_call (SMBPARSER_DBUS_SRV, SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "SectionUsed");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, section,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &section,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);
  dbus_error_init (&error);
  if((reply = dbus_connection_send_with_reply_and_block (bus, message, -1, &error)))
    {
      dbus_error_init (&error_args);
      if(!dbus_message_get_args (reply, &error_args, DBUS_TYPE_UINT32, &res, DBUS_TYPE_INVALID))
	g_print("SectionUsed error in dbus_message_get_args: %s\n",error_args.message);
      dbus_message_unref (reply);
      dbus_error_free (&error_args);
    }
  else
    g_print("SectionUsed error in reply: %s\n",error.message);
  dbus_error_free (&error);
  dbus_message_unref (message);
  return res;
}
/******************************************************************************/
gchar * 
smbparser_dbus_get_key(DBusConnection *bus, gchar *section, gchar *key)
{
  DBusMessage *message;
  DBusMessage *reply;
  DBusError   error;
  DBusError   error_args;
  char	*dbus_string;
  gchar	*value = NULL;

  if(!section || !key)
    return NULL;

  message = dbus_message_new_method_call (SMBPARSER_DBUS_SRV, SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "GetKey");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, section,
                            DBUS_TYPE_STRING, key,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &section,
                            DBUS_TYPE_STRING, &key,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);
  dbus_error_init (&error);
  if((reply = dbus_connection_send_with_reply_and_block (bus, message, -1, &error)))
    {
      dbus_error_init (&error_args);
      if (dbus_message_get_args (reply, &error_args, DBUS_TYPE_STRING, &dbus_string, DBUS_TYPE_INVALID))
	{
	  value = ((dbus_string == NULL) || (strcmp(dbus_string,"")==0)  ? NULL : g_strdup (dbus_string));
#ifndef DBUS_USE_NEW_API
	  dbus_free (dbus_string);
#endif /* ! DBUS_USE_NEW_API */
	}
      else
	g_print("GetKey error in dbus_message_get_args: %s\n",error_args.message);
      dbus_message_unref (reply);
      dbus_error_free (&error_args);
    }
  else
    g_print("GetKey error in reply: %s\n",error.message);
  dbus_error_free (&error);
  dbus_message_unref (message);
  return value;
}

/******************************************************************************/
gchar * 
smbparser_dbus_share_get_key(DBusConnection *bus, gchar *path, gchar *key)
{
  DBusMessage *message;
  DBusMessage *reply;
  DBusError   error;
  DBusError   error_args;
  char	*dbus_string;
  gchar	*value = NULL;

  if(!path || !key)
    return NULL;

  message = dbus_message_new_method_call (SMBPARSER_DBUS_SRV, SMBPARSER_DBUS_PATH, SMBPARSER_DBUS_INTERFACE, "ShareGetKey");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, path,
                            DBUS_TYPE_STRING, key,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &path,
                            DBUS_TYPE_STRING, &key,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);
  dbus_error_init (&error);
  if((reply = dbus_connection_send_with_reply_and_block (bus, message, -1, &error)))
    {
      dbus_error_init (&error_args);
      if (dbus_message_get_args (reply, &error_args, DBUS_TYPE_STRING, &dbus_string, DBUS_TYPE_INVALID))
	{
	  value = ((dbus_string == NULL) || (strcmp(dbus_string,"")==0)  ? NULL : g_strdup (dbus_string));
#ifndef DBUS_USE_NEW_API
	  dbus_free (dbus_string);
#endif /* ! DBUS_USE_NEW_API */
	}
      else
	g_print("GetKey error in dbus_message_get_args: %s\n",error_args.message);
      dbus_message_unref (reply);
      dbus_error_free (&error_args);
    }
  else
    g_print("GetKey error in reply: %s\n",error.message);
  dbus_error_free (&error);
  dbus_message_unref (message);
  return value;
}

/******************************************************************************/
gchar * 
smbparser_dbus_share_get_name(DBusConnection *bus, gchar *path)
{
  DBusMessage *message;
  DBusMessage *reply;
  DBusError   error;
  DBusError   error_args;
  char	*dbus_string;
  gchar	*value = NULL;

  message = dbus_message_new_method_call (SMBPARSER_DBUS_SRV, SMBPARSER_DBUS_PATH,
                                     SMBPARSER_DBUS_INTERFACE, "ShareGetName");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, path,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &path,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);

  dbus_error_init (&error);
  if((reply = dbus_connection_send_with_reply_and_block (bus, message, -1, &error)))
    {
      dbus_error_init (&error_args);
      if (dbus_message_get_args (reply, &error_args, DBUS_TYPE_STRING, &dbus_string, DBUS_TYPE_INVALID))
	{
	  value = ((dbus_string == NULL) || (strcmp(dbus_string,"")==0) ? NULL : g_strdup (dbus_string));
#ifndef DBUS_USE_NEW_API
	  dbus_free (dbus_string);
#endif /* ! DBUS_USE_NEW_API */
	}
      else
	g_print("ShareGetName error in dbus_message_get_args: %s\n",error_args.message);
      dbus_message_unref (reply);
      dbus_error_free (&error_args);
    }
  else
    g_print("ShareGetName error in reply: %s\n",error.message);
  dbus_error_free (&error);
  dbus_message_unref (message);
  return value;
}


/******************************************************************************/
gboolean  
smbparser_dbus_share_is_writable(DBusConnection *bus, gchar *path)
{
  DBusMessage	*message;
  DBusMessage	*reply;
  DBusError	error;
  DBusError	error_args;
  gboolean	dbus_boolean;

  message = dbus_message_new_method_call (SMBPARSER_DBUS_SRV, SMBPARSER_DBUS_PATH,
                                     SMBPARSER_DBUS_INTERFACE, "ShareIsWritable");
  dbus_message_append_args (message,
#ifndef DBUS_USE_NEW_API
                            DBUS_TYPE_STRING, path,
#else /* DBUS_USE_NEW_API */
                            DBUS_TYPE_STRING, &path,
#endif /* DBUS_USE_NEW_API */
                            DBUS_TYPE_INVALID);
  dbus_error_init (&error);
  if((reply = dbus_connection_send_with_reply_and_block (bus, message, -1, &error)))
    {
      dbus_error_init (&error_args);
      if(!dbus_message_get_args (reply, &error_args, DBUS_TYPE_BOOLEAN, &dbus_boolean, DBUS_TYPE_INVALID))
	g_print("ShareIsWritable error in dbus_message_get_args: %s\n",error_args.message);
      dbus_message_unref (reply);
      dbus_error_free (&error_args);
    }
  else
    g_print("ShareIsWritable error in reply: %s\n",error.message);
  dbus_error_free (&error);
  dbus_message_unref (message);
  return dbus_boolean;
}
/******************************************************************************/
#ifdef TEST
#include "smbparser-dbus-client.h"
#include "smbparser-dbus.h"

#include <gtk/gtk.h>
#include <glade/glade.h>

enum
  {
    SHARENAME_COL,
/*     USER_COL, */
    PATH_COL,
    N_COLUMNS
  };

/******************************************************************************/
typedef struct {
  GtkListStore *list_store;
  GtkTreeIter iter;
  DBusConnection *bus;
} MyList;

void
dump(char *path, gpointer user_data)
{
  MyList *mylist = (MyList *)user_data;

  gtk_list_store_append(mylist->list_store, &mylist->iter);
  gtk_list_store_set(mylist->list_store,&mylist->iter,
		     SHARENAME_COL,smbparser_dbus_share_get_name(mylist->bus,path),
/* 		     USER_COL,"sebest", */
		     PATH_COL,path,
		     -1);

}

int
main (int argc, char **argv)
{
  GMainLoop *loop;
  DBusConnection *bus;
  DBusError error;
  GSList *paths;

  GladeXML *xml;
  GtkTreeView *treeview_share_list;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkListStore *list_store;
  GtkTreeIter iter;
  
  gtk_init(&argc, &argv);
  glade_init();

  MyList *mylist = (MyList *)malloc(sizeof(MyList)); 

  xml = glade_xml_new("test/share-editor.glade",NULL,NULL);
  treeview_share_list = (GtkTreeView *)glade_xml_get_widget(xml,"treeview_share_list");


  list_store = gtk_list_store_new (N_COLUMNS,
				   G_TYPE_STRING, /* sharename */
/* 				   G_TYPE_STRING, /\* user *\/ */
				   G_TYPE_STRING /* path */
				   );
  
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview_share_list), GTK_TREE_MODEL (list_store));

/* share name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("share name",
						     renderer,
						     "text",
						     SHARENAME_COL,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview_share_list), column);

/* user name */
/*   renderer = gtk_cell_renderer_text_new (); */
/*   column = gtk_tree_view_column_new_with_attributes ("user name", */
/* 						     renderer, */
/* 						     "text", */
/* 						     USER_COL, */
/* 						     NULL); */
/*   gtk_tree_view_append_column (GTK_TREE_VIEW (treeview_share_list), column); */

/* path */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("path",
						     renderer,
						     "text",
						     PATH_COL,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview_share_list), column);
  /* Create a new event loop to run in */
  loop = g_main_loop_new (NULL, FALSE);

  /* Get a connection to the session bus */
  dbus_error_init (&error);
  bus = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
  if (!bus) {
    g_warning ("Failed to connect to the D-BUS daemon: %s", error.message);
    dbus_error_free (&error);
    return 1;
  }
  dbus_error_free (&error);

#ifndef DBUS_USE_NEW_API
  dbus_bus_acquire_service (bus, NAUTILUS_DBUS_SRV, 0, NULL);
#else /* DBUS_USE_NEW_API */
  dbus_bus_request_name (bus, NAUTILUS_DBUS_SRV, 0, NULL);
#endif /* DBUS_USE_NEW_API */
                            
  /* Set up this connection to work in a GLib event loop */
  dbus_connection_setup_with_g_main (bus, NULL);

  mylist->list_store = list_store;
  mylist->iter = iter;
  mylist->bus = bus;
  
  paths = smbparser_dbus_get_sharepaths(bus); 
  g_slist_foreach(paths,(GFunc)dump,mylist);

  gtk_main();
  return 0;
}
#endif
