#include <config.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include "shares.h"

static GHashTable *path_share_info_hash;
static GHashTable *share_name_share_info_hash;

/* Debugging flags */
static gboolean throw_error_on_add;
static gboolean throw_error_on_modify;
static gboolean throw_error_on_remove;



/* Internals */

static void
ensure_hashes (void)
{
	if (path_share_info_hash == NULL) {
		g_assert (share_name_share_info_hash == NULL);

		path_share_info_hash = g_hash_table_new (g_str_hash, g_str_equal);
		share_name_share_info_hash = g_hash_table_new (g_str_hash, g_str_equal);
	} else
		g_assert (share_name_share_info_hash != NULL);
}

static ShareInfo *
lookup_share_by_path (const char *path)
{
	ensure_hashes ();
	return g_hash_table_lookup (path_share_info_hash, path);
}

static ShareInfo *
lookup_share_by_share_name (const char *share_name)
{
	ensure_hashes ();
	return g_hash_table_lookup (share_name_share_info_hash, share_name);
}

#if 0
static gboolean
remove_from_path_hash_cb (gpointer key,
			  gpointer value,
			  gpointer data)
{
	ShareInfo *info;

	info = value;
	shares_free_share_info (info);

	return TRUE;
}

static gboolean
remove_from_share_name_hash_cb (gpointer key,
				gpointer value,
				gpointer data)
{
	/* The ShareInfo was already freed in remove_from_path_hash_cb() */
	return TRUE;
}

static void
free_all_shares (void)
{
	ensure_hashes ();
	g_hash_table_foreach_remove (path_share_info_hash, remove_from_path_hash_cb, NULL);
	g_hash_table_foreach_remove (share_name_share_info_hash, remove_from_share_name_hash_cb, NULL);
}

static gboolean
refresh_shares (GError **error)
{
	g_assert (error == NULL || *error == NULL);

	free_all_shares ();

	/* FIXME: use "net share" */

	return TRUE;
}
#endif

static ShareInfo *
copy_share_info (ShareInfo *info)
{
	ShareInfo *copy;

	if (!info)
		return NULL;

	copy = g_new (ShareInfo, 1);
	copy->path = g_strdup (info->path);
	copy->share_name = g_strdup (info->share_name);
	copy->comment = g_strdup (info->comment);
	copy->is_writable = info->is_writable;

	return copy;
}

static gboolean
add_share (ShareInfo *info, GError **error)
{
	ShareInfo *copy;

	if (throw_error_on_add) {
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_FAILED,
			     _("Failed"));
		return FALSE;
	}

	/* FIXME: remove the following and use "net share" */

	copy = copy_share_info (info);
	g_hash_table_insert (path_share_info_hash, copy->path, copy);

	return TRUE;
}

static gboolean
modify_share (const char *old_path, ShareInfo *info, GError **error)
{
	ShareInfo *old_info;

	old_info = lookup_share_by_path (old_path);
	if (old_info == NULL)
		return add_share (info, error);

	g_assert (old_info != NULL);

	if (strcmp (info->path, old_info->path) != 0) {
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_FAILED,
			     _("Cannot change the path of an existing share; please remove the old share first and add a new one"));
		return FALSE;
	}

	if (throw_error_on_modify) {
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_FAILED,
			     "Failed");
		return FALSE;
	}

	/* FIXME: remove the following and use "net share" */

	if (strcmp (old_info->share_name, info->share_name) != 0) {
		g_free (old_info->share_name);
		old_info->share_name = g_strdup (info->share_name);
	}

	if (strcmp (old_info->comment, info->comment) != 0) {
		g_free (old_info->comment);
		old_info->comment = g_strdup (info->comment);
	}

	if (old_info->is_writable != info->is_writable)
		old_info->is_writable = info->is_writable;

	return TRUE;
}

static gboolean
remove_share (const char *path, GError **error)
{
	ShareInfo *old_info;

	if (throw_error_on_remove) {
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_FAILED,
			     "Failed");
		return FALSE;
	}

	old_info = lookup_share_by_path (path);
	if (!old_info) {
		char *display_name;

		display_name = g_filename_display_name (path);
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_NONEXISTENT,
			     _("Cannot remove the share for path %s: that path is not shared"),
			     display_name);
		g_free (display_name);

		return FALSE;
	}

	/* FIXME: remove all the following and use "net share" */

	g_hash_table_remove (path_share_info_hash, old_info->path);
	shares_free_share_info (old_info);

	return TRUE;
}



/* Public API */

GQuark
shares_error_quark (void)
{
	static GQuark quark;

	if (quark == 0)
		quark = g_quark_from_string ("nautilus-shares-error-quark"); /* not from_static_string since we are a module */

	return quark;
}

/**
 * shares_free_share_info:
 * @info: A #ShareInfo structure.
 * 
 * Frees a #ShareInfo structure.
 **/
void
shares_free_share_info (ShareInfo *info)
{
	g_assert (info != NULL);

	g_free (info->path);
	g_free (info->share_name);
	g_free (info->comment);
	g_free (info);
}

/**
 * shares_get_path_is_shared:
 * @path: A full path name ("/foo/bar/baz") in file system encoding.
 * 
 * Return value: TRUE if the specified path is shared, FALSE otherwise.
 **/
gboolean
shares_get_path_is_shared (const char *path)
{
	return lookup_share_by_path (path) != NULL;
}

/**
 * shares_get_share_info_for_path:
 * @path: A full path name ("/foo/bar/baz") in file system encoding.
 * 
 * Queries the information for a shared path:  its share name, its read-only status, etc.
 * 
 * Return value: A #ShareInfo structure with the share information for the specified @path,
 * or %NULL if the path is not shared.  You must free this structure with shares_free_share_info().
 **/
ShareInfo *
shares_get_share_info_for_path (const char *path)
{
	ShareInfo *info;

	g_assert (path != NULL);

	info = lookup_share_by_path (path);
	return copy_share_info (info);
}

/**
 * shares_get_share_name_exists:
 * @share_name: Name of a share.
 * 
 * Return value: TRUE if a share with a name of @share_name exists, FALSE otherwise.
 **/
gboolean
shares_get_share_name_exists (const char *share_name)
{
	return lookup_share_by_share_name (share_name) != NULL;
}

/**
 * shares_get_share_info_for_share_name:
 * @share_name: Name of a share.
 * 
 * Queries the information for the share which has a specific name.
 * 
 * Return value: A #ShareInfo structure with the share information for the specified @share_name,
 * or %NULL if no share has such name.  You must free this structure with shares_free_share_info().
 **/
ShareInfo *
shares_get_share_info_for_share_name (const char *share_name)
{
	ShareInfo *info;

	g_assert (share_name != NULL);

	info = lookup_share_by_share_name (share_name);
	return copy_share_info (info);
}

/**
 * shares_modify_share:
 * @old_path: Path of the share to modify, or %NULL.
 * @info: Info of the share to modify/add, or %NULL to delete a share.
 * @error: Location to store error.
 * 
 * Can add, modify, or delete shares.  To add a share, pass %NULL for @old_path, and a non-null @info.
 * To modify a share, pass a non-null @old_path and non-null @info.  To remove a share, pass a non-null
 * @old_path and a %NULL @info.
 * 
 * Return value: TRUE if the share could be modified, FALSE otherwise.  If this returns
 * FALSE, then the error information will be placed in @error.
 **/
gboolean
shares_modify_share (const char *old_path, ShareInfo *info, GError **error)
{
	g_assert ((old_path == NULL && info != NULL)
		  || (old_path != NULL && info == NULL)
		  || (old_path != NULL && info != NULL));
	g_assert (error == NULL || *error == NULL);

	if (old_path == NULL)
		return add_share (info, error);
	else if (info == NULL)
		return remove_share (old_path, error);
	else
		return modify_share (old_path, info, error);
}

static void
copy_to_slist_cb (gpointer key, gpointer value, gpointer data)
{
	ShareInfo *info;
	ShareInfo *copy;
	GSList **list;

	info = value;
	list = data;

	copy = copy_share_info (info);
	*list = g_slist_prepend (*list, copy);
}

/**
 * shares_get_share_info_list:
 *
 * Gets the list of shared folders and their information.
 * 
 * Return value: A list of #ShareInfo structures.  You must free this list with
 * shares_free_share_info_list().
 **/
GSList *
shares_get_share_info_list (void)
{
	GSList *result;

	result = NULL;
	g_hash_table_foreach (path_share_info_hash, copy_to_slist_cb, &result);
	return result;
}

/**
 * shares_free_share_info_list:
 * @list: List of #ShareInfo structures, or %NULL.
 * 
 * Frees a list of #ShareInfo structures as returned by shares_get_share_info_list().
 **/
void
shares_free_share_info_list (GSList *list)
{
	GSList *l;

	for (l = list; l; l = l->next) {
		ShareInfo *info;

		info = l->data;
		shares_free_share_info (l->data);
	}

	g_slist_free (list);
}

void
shares_set_debug (gboolean error_on_add,
		  gboolean error_on_modify,
		  gboolean error_on_remove)
{
	throw_error_on_add = error_on_add;
	throw_error_on_modify = error_on_modify;
	throw_error_on_remove = error_on_remove;
}
