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

#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-column-provider.h>
#include <libnautilus-extension/nautilus-extension-types.h>
#include <libnautilus-extension/nautilus-file-info.h>
#include <libnautilus-extension/nautilus-info-provider.h>
#include <libnautilus-extension/nautilus-menu-provider.h>
#include <libnautilus-extension/nautilus-property-page-provider.h>

#include <libgnomevfs/gnome-vfs-utils.h>
#include "nautilus-share.h"

#include <eel/eel-vfs-extensions.h>

#include <glib/gi18n-lib.h>

#include <gtk/gtktable.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtk.h>
#include <gtk/gtkentry.h>

#include <glade/glade.h>

#include <string.h>
#include <time.h>

#include <unistd.h>
#include <stdlib.h>

#include "smbparser-dbus-client.h"
#include "smbparser-dbus.h"

static GObjectClass *parent_class;
DBusConnection *g_dbus;
GladeXML *xml;
gchar *g_sharename = NULL;


/*--------------------------------------------------------------------------*/
/* Share Editor */
enum
  {
    SHARENAME_COL,
/*     USER_COL, */
    PATH_COL,
    N_COLUMNS
  };

typedef struct {
  GtkListStore *list_store;
  GtkTreeIter iter;
} MyList;

void
dump(char *path, gpointer user_data)
{
  MyList *mylist = (MyList *)user_data;

  gtk_list_store_append(mylist->list_store, &mylist->iter);
  gtk_list_store_set(mylist->list_store,&mylist->iter,
		     SHARENAME_COL,smbparser_dbus_share_get_name(g_dbus,path),
/* 		     USER_COL,"sebest", */
		     PATH_COL,path,
		     -1);

}

int
shareeditor (void)
{
  GSList *paths;

  GtkTreeView *treeview_share_list;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkListStore *list_store;
  GtkTreeIter iter;
  
  glade_init();

  MyList *mylist = (MyList *)malloc(sizeof(MyList)); 

  treeview_share_list = (GtkTreeView *)glade_xml_get_widget(xml,"treeview_share_list");


  list_store = gtk_list_store_new (N_COLUMNS,
				   G_TYPE_STRING, /* sharename */
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

/* path */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("path",
						     renderer,
						     "text",
						     PATH_COL,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview_share_list), column);

  mylist->list_store = list_store;
  mylist->iter = iter;
  
  paths = smbparser_dbus_get_sharepaths(g_dbus); 
  g_slist_foreach(paths,(GFunc)dump,mylist);

  return 0;
}


/*--------------------------------------------------------------------------*/
gchar *
get_fullpath_from_fileinfo(NautilusFileInfo *fileinfo)
{
  gchar *uri;
  gchar *fullpath = NULL;
  
  if (fileinfo)
    {
      uri = nautilus_file_info_get_uri(fileinfo);
      fullpath = gnome_vfs_get_local_path_from_uri(uri);
      g_free(uri);
    }
  return(fullpath);
}
/*--------------------------------------------------------------------------*/
gboolean
left_share_comment_text_entry  (GtkWidget *widget,
				GdkEventCrossing *event,
				gpointer user_data)
{
  gchar *fullpath = (gchar *)user_data;
  char *comment = (char *)gtk_entry_get_text((GtkEntry *)widget);

  if(fullpath && comment)
    {
      /* check that the comment field is not empty */
      if(strcmp(comment, ""))
	smbparser_dbus_share_set_key(g_dbus,fullpath,"comment",comment);
      else
	smbparser_dbus_share_remove_key(g_dbus,fullpath,"comment");
    }
  smbparser_dbus_write(g_dbus);
  return FALSE;
}

/*--------------------------------------------------------------------------*/
void
set_warning(GtkWidget *widget)
{
  GdkColor   colorYellow;
  GtkWidget *label_status = glade_xml_get_widget(xml,"label_status");
  gtk_label_set_text((GtkLabel *)label_status, _("Sharename too long, may not appear on all Os"));
  gdk_color_parse ("#ECDF62", &colorYellow);
  gtk_widget_modify_base(widget,GTK_STATE_NORMAL, &colorYellow);
}


void
set_error(GtkWidget *widget)
{
  GdkColor   colorRed;
  GtkWidget *label_status = glade_xml_get_widget(xml,"label_status");
  gtk_label_set_text((GtkLabel *)label_status, _("Sharename already in use or forbidden!"));
  gdk_color_parse ("#C1665A", &colorRed);
  gtk_widget_modify_base(widget,GTK_STATE_NORMAL, &colorRed);
}

void
set_normal(GtkWidget *widget)
{
  GtkWidget *label_status = glade_xml_get_widget(xml,"label_status");
  gtk_label_set_text((GtkLabel *)label_status, "");
  gtk_widget_modify_base(widget,GTK_STATE_NORMAL, NULL);
}

gboolean
modify_share_name_text_entry  (GtkWidget *widget,
			     gpointer user_data)
{
  gchar *newname;
  newname = (char *)gtk_entry_get_text((GtkEntry *)widget);

 
  if(!g_sharename || ! newname)
    {
      return FALSE;
    }
  /* not share name given */
  if(strlen(newname) == 0)
    {
      set_error(widget);
      return FALSE;
    }
  /* sharename is already in used */
  if(smbparser_dbus_section_used(g_dbus,newname) == TRUE)
    {
      /* and the new name is not the same as the old one */
      if(strcmp(newname,g_sharename) != 0)
	{
	  set_error(widget);
	  return FALSE;
	}
    }
  /*sharename is longer than 12 characters, may have problem with some implementations of smb */
  if(strlen(newname) <= 12)
    set_normal(widget);
  else
    set_warning(widget);
  return FALSE;
}
/*--------------------------------------------------------------------------*/
void
on_checkbutton_share_folder_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  NautilusFileInfo *fileinfo = (NautilusFileInfo *)user_data;
  gchar *fullpath = get_fullpath_from_fileinfo(fileinfo);
  GtkWidget *entry_share_comment = glade_xml_get_widget(xml,"entry_share_comment");
  GtkWidget *checkbutton_share_rw_ro = glade_xml_get_widget(xml,"checkbutton_share_rw_ro");
  GtkWidget *entry_share_name = glade_xml_get_widget(xml,"entry_share_name");
  GtkWidget *hbox_share_comment = glade_xml_get_widget(xml,"hbox_share_comment");
  GtkWidget *hbox_share_name = glade_xml_get_widget(xml,"hbox_share_name");

  /* FIXME free? */
  gchar *tmp;
  
  if(gtk_toggle_button_get_active (togglebutton))
    {
      char *name = (char *)gtk_entry_get_text((GtkEntry *)entry_share_name);
      /* sharing button is active so we make things editable */
      gtk_widget_set_sensitive (entry_share_name, TRUE);
      gtk_widget_set_sensitive (entry_share_comment, TRUE);
      gtk_widget_set_sensitive (hbox_share_comment, TRUE);
      gtk_widget_set_sensitive (hbox_share_name, TRUE);
      gtk_widget_set_sensitive (checkbutton_share_rw_ro,TRUE);

      if (smbparser_dbus_share_add(g_dbus,fullpath,name) == 0)
	{
	  if (strlen(name) <= 12)
	    set_normal((GtkWidget *)entry_share_name);
	  else
	    set_warning((GtkWidget *)entry_share_name);
	  /* 
	     add_section return 0:
	     - we create a new share gathering infos from the form
	  */
	  /* set the comment */
	  if((tmp = (gchar *)gtk_entry_get_text((GtkEntry *)entry_share_comment)))
	    if(strcmp(tmp,""))
	      smbparser_dbus_share_set_key(g_dbus,fullpath,"comment",tmp);
	  /* set writable state */
	  if(gtk_toggle_button_get_active((GtkToggleButton *)checkbutton_share_rw_ro))
	    smbparser_dbus_share_set_key(g_dbus,fullpath,"writable","yes");
	  else
	    smbparser_dbus_share_set_key(g_dbus,fullpath,"writable","no");
	  /* always set public */
	  smbparser_dbus_share_set_key(g_dbus,fullpath,"public","yes");
	  /* write the result in config file */
	  smbparser_dbus_write(g_dbus);
	  /* ask nautilus to update emblem */
	  nautilus_file_info_invalidate_extension_info (fileinfo);
	}
      /* 
	 else
	 add_section return 1;
	 - the sharename is already in used
      */
      else
	{
	  set_error((GtkWidget *)entry_share_name);
	}
    }
  else
    {
      /*  sharing button is inactive */
      /* make form UNeditable (greyed) */
      gtk_widget_set_sensitive (hbox_share_comment, FALSE);
      gtk_widget_set_sensitive (hbox_share_name, FALSE);
      gtk_widget_set_sensitive (checkbutton_share_rw_ro,FALSE);
      gtk_widget_set_sensitive (entry_share_name, FALSE);
      gtk_widget_set_sensitive (entry_share_comment, FALSE);
      /* we remove the current section */
      smbparser_dbus_share_remove(g_dbus,fullpath);
      /* write the result in config file */
      smbparser_dbus_write(g_dbus);
      /* ask nautilus to update emblem */
      nautilus_file_info_invalidate_extension_info (fileinfo);
    }
  g_free(fullpath);
}


/*--------------------------------------------------------------------------*/
gboolean
left_share_name_text_entry  (GtkWidget *widget,
			     GdkEventCrossing *event,
			     gpointer user_data)
{
  NautilusFileInfo *fileinfo = (NautilusFileInfo *)user_data;
  gchar *fullpath = get_fullpath_from_fileinfo(fileinfo);
  GtkWidget *entry_share_comment = glade_xml_get_widget(xml,"entry_share_comment");
  GtkWidget *checkbutton_share_rw_ro = glade_xml_get_widget(xml,"checkbutton_share_rw_ro");
  /*   GtkWidget *entry_share_name = glade_xml_get_widget(xml,"entry_share_name"); */
  gchar *comment;

/*   GtkWidget *checkbutton_share_folder = w->checkbutton_share_folder; */

  char *newname = (char *)gtk_entry_get_text((GtkEntry *)widget);
  if (newname && strcmp(newname,"") && ! smbparser_dbus_section_used(g_dbus,newname))
    {
      if (smbparser_dbus_share_rename(g_dbus,fullpath,newname) == 2)
	{
	  if (smbparser_dbus_share_add(g_dbus,fullpath,newname) == 0)
	    {
	      if((comment = (gchar *)gtk_entry_get_text((GtkEntry *)entry_share_comment)))
		if(strcmp(comment,""))
		  smbparser_dbus_share_set_key(g_dbus,fullpath,"comment",comment);
	      if(gtk_toggle_button_get_active((GtkToggleButton *)checkbutton_share_rw_ro))
		smbparser_dbus_share_set_key(g_dbus,fullpath,"writable","yes");
	      else
		smbparser_dbus_share_set_key(g_dbus,fullpath,"writable","no");
	      smbparser_dbus_share_set_key(g_dbus,fullpath,"public","yes");
	      smbparser_dbus_write(g_dbus);
	      /* FIXME do we need it? */
	      nautilus_file_info_invalidate_extension_info (fileinfo);
	    }
	}
      smbparser_dbus_write(g_dbus);
    }
  g_free(fullpath);
  return FALSE;
}

/*--------------------------------------------------------------------------*/
void
on_checkbutton_share_rw_ro_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  gchar *fullpath = (gchar *)user_data;

  smbparser_dbus_share_set_key(g_dbus,fullpath,"writable",
			 gtk_toggle_button_get_active (togglebutton)?"yes":"no");
  smbparser_dbus_write(g_dbus);
}

/*--------------------------------------------------------------------------*/
static GtkWidget *
create_property_page (NautilusFileInfo *fileinfo)
{
  GtkWidget * page;

  GtkWidget *checkbutton_share_folder;
  GtkWidget *hbox_share_name;
  GtkWidget *hbox_share_comment;
  GtkWidget *entry_share_name;
  GtkWidget *checkbutton_share_rw_ro;
  GtkWidget *entry_share_comment;

  char *comment;
  char *fullpath = get_fullpath_from_fileinfo(fileinfo);

  xml = glade_xml_new(INTERFACES_DIR"/share-dialog.glade","vbox1",GETTEXT_PACKAGE);
  
  page = glade_xml_get_widget(xml,"vbox1");
  checkbutton_share_folder = glade_xml_get_widget(xml,"checkbutton_share_folder");
  hbox_share_comment = glade_xml_get_widget(xml,"hbox_share_comment");
  hbox_share_name = glade_xml_get_widget(xml,"hbox_share_name");
  checkbutton_share_rw_ro = glade_xml_get_widget(xml,"checkbutton_share_rw_ro");
  entry_share_name = glade_xml_get_widget(xml,"entry_share_name");
  entry_share_comment = glade_xml_get_widget(xml,"entry_share_comment");

  g_free(g_sharename);

  if(!(g_sharename  = smbparser_dbus_share_get_name(g_dbus,fullpath)))
    g_sharename = g_path_get_basename(fullpath);

  gtk_entry_set_text((GtkEntry *)entry_share_name, g_sharename);

  comment = smbparser_dbus_share_get_key(g_dbus,fullpath, "comment");
  gtk_entry_set_text((GtkEntry *)entry_share_comment,comment?comment:"");

  g_signal_connect ((gpointer) entry_share_comment,"focus-out-event",
                    G_CALLBACK (left_share_comment_text_entry),
                    fullpath);

  g_signal_connect ((gpointer) entry_share_name, "focus-out-event",
                    G_CALLBACK (left_share_name_text_entry),
                    fileinfo);

  g_signal_connect ((gpointer) entry_share_name, "key-release-event",
                    G_CALLBACK (modify_share_name_text_entry),
                    NULL);

  g_signal_connect ((gpointer) checkbutton_share_folder, "toggled",
                    G_CALLBACK (on_checkbutton_share_folder_toggled),
                    fileinfo);

  g_signal_connect ((gpointer) checkbutton_share_rw_ro, "toggled",
                    G_CALLBACK (on_checkbutton_share_rw_ro_toggled),
                    fullpath);

  
  if (file_get_share_status (fullpath) != NAUTILUS_SHARE_NOT_SHARED)
    {
      GTK_TOGGLE_BUTTON (checkbutton_share_folder)->active = TRUE;
    }
  else
    {
      gtk_widget_set_sensitive (hbox_share_comment, FALSE);
      gtk_widget_set_sensitive (hbox_share_name, FALSE);
      gtk_widget_set_sensitive (checkbutton_share_rw_ro,FALSE);
      gtk_widget_set_sensitive (entry_share_name, FALSE);
      gtk_widget_set_sensitive (entry_share_comment, FALSE);
    }

  if (strlen((char *)gtk_entry_get_text((GtkEntry *)entry_share_name)) > 12)
    set_warning((GtkWidget *)entry_share_name);

  if (file_get_share_status (fullpath) == NAUTILUS_SHARE_SHARED_RW)
    {
      GTK_TOGGLE_BUTTON (checkbutton_share_rw_ro)->active = TRUE;
    }

/*   shareeditor(); */
/*   g_free(fullpath); */
  return page;
}

/* Implementation of the NautilusInfoProvider interface */

/* nautilus_info_provider_update_file_info 
 * This function is called by Nautilus when it wants the extension to 
 * fill in data about the file.  It passes a NautilusFileInfo object,
 * which the extension can use to read data from the file, and which
 * the extension should add data to.
 *
 * If the data can be added immediately (without doing blocking IO), 
 * the extension can do so, and return NAUTILUS_OPERATION_COMPLETE.  
 * In this case the 'update_complete' and 'handle' parameters can be 
 * ignored.
 * 
 * If waiting for the deata would block the UI, the extension should
 * perform the task asynchronously, and return 
 * NAUTILUS_OPERATION_IN_PROGRESS.  The function must also set the 
 * 'handle' pointer to a value unique to the object, and invoke the
 * 'update_complete' closure when the update is done.
 * 
 * If the extension encounters an error, it should return 
 * NAUTILUS_OPERATION_FAILED.
 */
typedef struct {
  gboolean cancelled;
  NautilusInfoProvider *provider;
  NautilusFileInfo *file;
  GClosure *update_complete;
} NautilusShareHandle;



/*--------------------------------------------------------------------------*/
static NautilusShareStatus 
file_get_share_status (gchar *fullpath) {
  NautilusShareStatus res;

  if( smbparser_dbus_share_get_name(g_dbus,fullpath) == NULL)
    res = NAUTILUS_SHARE_NOT_SHARED;
  else
    if(smbparser_dbus_share_is_writable(g_dbus,fullpath) == TRUE)  
      res = NAUTILUS_SHARE_SHARED_RW;
    else
      res = NAUTILUS_SHARE_SHARED_RO;
  
  return res;
}

/*--------------------------------------------------------------------------*/
static NautilusShareStatus 
file_get_share_status_file(NautilusFileInfo *file)
{
  char		*path = NULL;
  char		*local_path = NULL;

  if (!nautilus_file_info_is_directory(file) ||
      !(path =  nautilus_file_info_get_uri(file)))
    {
      return NAUTILUS_SHARE_NOT_SHARED;
    }
  if(!(local_path = gnome_vfs_get_local_path_from_uri(path)))
    {
      g_free(path);
      return NAUTILUS_SHARE_NOT_SHARED;
    }

  /* FIXME bad UTF8 */
  if (!g_utf8_validate (local_path, -1, NULL)) {
    g_free(path);
    g_free(local_path);
    return NAUTILUS_SHARE_NOT_SHARED;
  }

  if ((strncmp(path,"file://",7)!=0) ||
      (smbparser_dbus_share_get_name(g_dbus,local_path) == NULL))
    {
      g_free(path);
      return NAUTILUS_SHARE_NOT_SHARED;
    }

  g_free(path);
  g_free(local_path);
  return NAUTILUS_SHARE_SHARED_RO;
}

static NautilusOperationResult
nautilus_share_update_file_info (NautilusInfoProvider *provider,
				 NautilusFileInfo *file,
				 GClosure *update_complete,
				 NautilusOperationHandle **handle)
{
/*   gchar *share_status = NULL; */
  
  switch (file_get_share_status_file (file)) {

  case NAUTILUS_SHARE_SHARED_RO:
    nautilus_file_info_add_emblem (file, "shared");
/*     share_status = _("shared (read only)"); */
    break;

  case NAUTILUS_SHARE_SHARED_RW:
    nautilus_file_info_add_emblem (file, "shared");
/*     share_status = _("shared (read and write)"); */
    break;

  case NAUTILUS_SHARE_NOT_SHARED:
/*     share_status = _("not shared"); */
    break;

  default:
    g_assert_not_reached ();
    break;
  }

/*   nautilus_file_info_add_string_attribute (file, */
/* 					   "NautilusShare::share_status", */
/* 					   share_status); */
  return NAUTILUS_OPERATION_COMPLETE;
}


static void
nautilus_share_cancel_update (NautilusInfoProvider *provider,
			      NautilusOperationHandle *handle)
{
  NautilusShareHandle *share_handle;
	
  share_handle = (NautilusShareHandle*)handle;
  share_handle->cancelled = TRUE;
}

static void 
nautilus_share_info_provider_iface_init (NautilusInfoProviderIface *iface)
{
  iface->update_file_info = nautilus_share_update_file_info;
  iface->cancel_update = nautilus_share_cancel_update;
}

/*--------------------------------------------------------------------------*/
/* nautilus_property_page_provider_get_pages
 *  
 * This function is called by Nautilus when it wants property page
 * items from the extension.
 *
 * This function is called in the main thread before a property page
 * is shown, so it should return quickly.
 * 
 * The function should return a GList of allocated NautilusPropertyPage
 * items.
 */
static GList *
nautilus_share_get_property_pages (NautilusPropertyPageProvider *provider,
				   GList *files)
{
  GList *pages;
  NautilusPropertyPage *page;
  char *tmp;
  NautilusFileInfo *fileinfo;

  /* Only show the property page if 1 file is selected */
  if (!files || files->next != NULL) {
    return NULL;
  }

  fileinfo = NAUTILUS_FILE_INFO (files->data);
  /* if it's not a directory it can't be shared */
  if (!nautilus_file_info_is_directory(fileinfo))
    return NULL;
  
  tmp = nautilus_file_info_get_uri_scheme(fileinfo);
  if (strncmp(tmp,"file",4) != 0)
    {
      g_free(tmp);
      return NULL;
    }
  g_free(tmp);
  
  pages = NULL;
  page = nautilus_property_page_new
    ("NautilusShare::property_page",
     gtk_label_new (_("Share")),
     create_property_page (fileinfo));
  pages = g_list_append (pages, page);

  return pages;
}

/*--------------------------------------------------------------------------*/
static void 
nautilus_share_property_page_provider_iface_init (NautilusPropertyPageProviderIface *iface)
{
  iface->get_pages = nautilus_share_get_property_pages;
}

/*--------------------------------------------------------------------------*/
static void
nautilus_share_instance_init (NautilusShare *share)
{
}

/*--------------------------------------------------------------------------*/
static void
nautilus_share_class_init (NautilusShareClass *class)
{
  parent_class = g_type_class_peek_parent (class);
}

/* nautilus_menu_provider_get_file_items
 *  
 * This function is called by Nautilus when it wants context menu
 * items from the extension.
 *
 * This function is called in the main thread before a context menu
 * is shown, so it should return quickly.
 * 
 * The function should return a GList of allocated NautilusMenuItem
 * items.
 */
static void button_callback( GtkWidget *widget,
                      gpointer   data )
{
  gtk_widget_destroy((GtkWidget *)data);
}

static void
share_this_folder_callback (NautilusMenuItem *item,
	      gpointer user_data)
{
  NautilusFileInfo *fileinfo =(NautilusFileInfo *)user_data;
  GtkWidget * page;
  GtkWidget * window;
  GtkWidget * button;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  page = create_property_page (fileinfo);
  button = glade_xml_get_widget(xml,"button_close");
  gtk_container_add (GTK_CONTAINER (window), page);
  gtk_widget_show (button);
  gtk_widget_show (window);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (button_callback), window);
}


static GList *
nautilus_share_get_file_items (NautilusMenuProvider *provider,
			     GtkWidget *window,
			     GList *files)
{
  GList *items = NULL;
  NautilusMenuItem *item;
  char *tmp;
  NautilusFileInfo *fileinfo;


  /* Only show the property page if 1 file is selected */
  if (!files || files->next != NULL) {
    return NULL;
  }

  fileinfo = NAUTILUS_FILE_INFO (files->data);
  /* if it's not a directory it can't be shared */
  if (!nautilus_file_info_is_directory(fileinfo))
    return NULL;
  
  tmp = nautilus_file_info_get_uri_scheme(fileinfo);
  if (strncmp(tmp,"file",4) != 0)
    {
      g_free(tmp);
      return NULL;
    }
  g_free(tmp);
  
  tmp = nautilus_file_info_get_uri(fileinfo);
  
  item = nautilus_menu_item_new ("NautilusShare::share",
				 _("Share"),
				 _("Share this Folder"),
				 "stock_shared-by-me");
  g_signal_connect (item, "activate",
		    G_CALLBACK (share_this_folder_callback),
		    fileinfo);
  g_object_set_data (G_OBJECT (item), 
		     "files",
		     nautilus_file_info_list_copy (files));
  
  items = g_list_append (items, item);
  return items;
}

/*--------------------------------------------------------------------------*/
static void 
nautilus_share_menu_provider_iface_init (NautilusMenuProviderIface *iface)
{
	iface->get_file_items = nautilus_share_get_file_items;
}

/*--------------------------------------------------------------------------*/
/* Type registration.  Because this type is implemented in a module
 * that can be unloaded, we separate type registration from get_type().
 * the type_register() function will be called by the module's
 * initialization function. */
static GType share_type = 0;

GType
nautilus_share_get_type (void) 
{
  return share_type;
}

void
nautilus_share_register_type (GTypeModule *module)
{
  static const GTypeInfo info = {
    sizeof (NautilusShareClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) nautilus_share_class_init,
    NULL, 
    NULL,
    sizeof (NautilusShare),
    0,
    (GInstanceInitFunc) nautilus_share_instance_init,
  };

  share_type = g_type_module_register_type (module,
					    G_TYPE_OBJECT,
					    "NautilusShare",
					    &info, 0);

  /* onglet share propriété */
  static const GInterfaceInfo property_page_provider_iface_info = {
    (GInterfaceInitFunc) nautilus_share_property_page_provider_iface_init,
    NULL,
    NULL
  };
	
  g_type_module_add_interface (module,
			       share_type,
			       NAUTILUS_TYPE_PROPERTY_PAGE_PROVIDER,
			       &property_page_provider_iface_info);


  /* premier page propriété ? */
  static const GInterfaceInfo info_provider_iface_info = {
    (GInterfaceInitFunc) nautilus_share_info_provider_iface_init,
    NULL,
    NULL
  };

  g_type_module_add_interface (module,
			       share_type,
			       NAUTILUS_TYPE_INFO_PROVIDER,
			       &info_provider_iface_info);

  /* Menu right clik */
  static const GInterfaceInfo menu_provider_iface_info = {
    (GInterfaceInitFunc) nautilus_share_menu_provider_iface_init,
    NULL,
    NULL
  };
  
  g_type_module_add_interface (module,
			       share_type,
			       NAUTILUS_TYPE_MENU_PROVIDER,
			       &menu_provider_iface_info);
  
}

/* Extension module functions.  These functions are defined in 
 * nautilus-extensions-types.h, and must be implemented by all 
 * extensions. */

/* Initialization function.  In addition to any module-specific 
 * initialization, any types implemented by the module should 
 * be registered here. */
void
nautilus_module_initialize (GTypeModule  *module)
{
  DBusError error;

  g_print ("Initializing nautilus-share extension\n");

  bindtextdomain("nautilus-share", NULL);
  bind_textdomain_codeset("nautilus-share", "UTF-8");

  dbus_error_init (&error);
  g_dbus = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
  if (!g_dbus) {
    g_warning ("Failed to connect to the D-BUS daemon: %s", error.message);
    dbus_error_free (&error);
    return;
  }
  dbus_error_free (&error);

  /* FIXME error */
/*   dbus_bus_set_base_service(g_dbus, NAUTILUS_DBUS_SRV); */
#ifndef DBUS_USE_NEW_API
  dbus_bus_acquire_service (g_dbus, NAUTILUS_DBUS_SRV,0, NULL);
#else /* DBUS_USE_NEW_API */
  dbus_bus_request_name (g_dbus, NAUTILUS_DBUS_SRV,0, NULL);
#endif /* DBUS_USE_NEW_API */
  
  dbus_connection_setup_with_g_main (g_dbus, NULL);

  nautilus_share_register_type (module);
}

/* Perform module-specific shutdown. */
void
nautilus_module_shutdown   (void)
{
  g_print ("Shutting down nautilus-share extension\n");
  /* FIXME freeing */
}

/* List all the extension types.  */
void 
nautilus_module_list_types (const GType **types,
			    int          *num_types)
{
  static GType type_list[1];
	
  type_list[0] = NAUTILUS_TYPE_SHARE;

  *types = type_list;
  *num_types = 1;
}
