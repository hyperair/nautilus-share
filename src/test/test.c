#include <gtk/gtk.h>
#include <glade/glade.h>

enum
  {
    SHARENAME_COL,
    USER_COL,
    PATH_COL,
    N_COLUMNS
  };

int main (int argc, char *argv[])
{
  GladeXML *xml;
  GtkTreeView *treeview_share_list;
  GtkTreeViewColumn *tree_column;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkListStore *list_store;
  GtkTreeIter iter;
  
  gtk_init(&argc, &argv);
  glade_init();

  xml = glade_xml_new("share-editor.glade",NULL,NULL);
  treeview_share_list = (GtkTreeView *)glade_xml_get_widget(xml,"treeview_share_list");


  list_store = gtk_list_store_new (N_COLUMNS,
				   G_TYPE_STRING, /* sharename */
				   G_TYPE_STRING, /* user */
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
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("user name",
						     renderer,
						     "text",
						     USER_COL,
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


  gtk_list_store_append(list_store, &iter);
  gtk_list_store_set(list_store,&iter,
		     SHARENAME_COL,"download1",
		     USER_COL,"sebest",
		     PATH_COL,"/home/sebest/download",
		     -1);

  gtk_list_store_append(list_store, &iter);
  gtk_list_store_set(list_store,&iter,
		     SHARENAME_COL,"download2",
		     USER_COL,"sebest",
		     PATH_COL,"/home/sebest/download",
		     -1);

  gtk_list_store_append(list_store, &iter);
  gtk_list_store_set(list_store,&iter,
		     SHARENAME_COL,"download3",
		     USER_COL,"sebest",
		     PATH_COL,"/home/sebest/download",
		     -1);

/*   g_object_unref (list_store); */
  gtk_main();
  return 0;
}
