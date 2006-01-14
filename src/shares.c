#include <config.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib/gi18n-lib.h>
#include "shares.h"

#define DEBUG_SHARES
#ifdef DEBUG_SHARES
#  define NET_USERSHARE_ARGV0 "debug-net-usershare"
#else
#  define NET_USERSHARE_ARGV0 "net"
#endif

static GHashTable *path_share_info_hash;
static GHashTable *share_name_share_info_hash;

#define NUM_CALLS_BETWEEN_TIMESTAMP_UPDATES 100
#define TIMESTAMP_THRESHOLD 10	/* seconds */
static int refresh_timestamp_update_counter;
static time_t refresh_timestamp;

#define KEY_PATH "path"
#define KEY_COMMENT "comment"
#define KEY_ACL "usershare_acl"

/* Debugging flags */
static gboolean throw_error_on_refresh;
static gboolean throw_error_on_add;
static gboolean throw_error_on_modify;
static gboolean throw_error_on_remove;



/* Interface to "net usershare" */

static gboolean
net_usershare_run (int argc, char **argv, GKeyFile **ret_key_file, GError **error)
{
	int real_argc;
	int i;
	char **real_argv;
	gboolean retval;
	char *stdout_contents;
	char *stderr_contents;
	int exit_status;
	int exit_code;
	GKeyFile *key_file;

	g_assert (argc > 0);
	g_assert (argv != NULL);
	g_assert (ret_key_file != NULL);
	g_assert (error == NULL || *error == NULL);

	/* Build command line */

	real_argc = 2 + argc + 1; /* "net" "usershare" [argv] NULL */
	real_argv = g_new (char *, real_argc);

	real_argv[0] = NET_USERSHARE_ARGV0;
	real_argv[1] = "usershare";

	for (i = 0; i < argc; i++) {
		g_assert (argv[i] != NULL);
		real_argv[i + 2] = argv[i];
	}

	real_argv[real_argc - 1] = NULL;

	/* Launch */

	stdout_contents = NULL;
	stderr_contents = NULL;
	*ret_key_file = NULL;

	retval = g_spawn_sync (NULL,			/* cwd */
			       real_argv,
			       NULL, 			/* envp */
			       G_SPAWN_SEARCH_PATH,
			       NULL, 			/* GSpawnChildSetupFunc */
			       NULL,			/* user_data */
			       &stdout_contents,
			       &stderr_contents,
			       &exit_status,
			       error);

	if (!retval)
		goto out;

	if (!WIFEXITED (exit_status))
		goto out;

	exit_code = WEXITSTATUS (exit_status);

	if (exit_code != 0) {
		char *str;

		/* stderr_contents is in the system locale encoding, not UTF-8 */

		str = g_locale_to_utf8 (stderr_contents, -1, NULL, NULL, NULL);
		g_set_error (error,
			     G_SPAWN_ERROR,
			     G_SPAWN_ERROR_FAILED,
			     _("'net usershare' returned error %d: %s"),
			     exit_code,
			     str ? str : "???");
		g_free (str);

		retval = FALSE;
		goto out;
	}

	/* FIXME: jeallison@novell.com says the output of "net usershare" is nearly always
	 * in UTF-8, but that it can be configured in the master smb.conf.  We assume
	 * UTF-8 for now.
	 */

	if (!g_utf8_validate (stdout_contents, -1, NULL)) {
		g_set_error (error,
			     G_SPAWN_ERROR,
			     G_SPAWN_ERROR_FAILED,
			     _("the output of 'net usershare' is not in valid UTF-8 encoding"));
		retval = FALSE;
		goto out;
	}

	key_file = g_key_file_new ();

	if (!g_key_file_load_from_data (key_file, stdout_contents, -1, 0, error)) {
		g_key_file_free (key_file);
		retval = FALSE;
		goto out;
	}

	retval = TRUE;
	*ret_key_file = key_file;

 out:
	g_free (real_argv);
	g_free (stdout_contents);
	g_free (stderr_contents);

	return retval;
}



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

static void
add_share_info_to_hashes (ShareInfo *info)
{
	ensure_hashes ();
	g_hash_table_insert (path_share_info_hash, info->path, info);
	g_hash_table_insert (share_name_share_info_hash, info->share_name, info);
}

static void
remove_share_info_from_hashes (ShareInfo *info)
{
	ensure_hashes ();
	g_hash_table_remove (path_share_info_hash, info->path);
	g_hash_table_remove (share_name_share_info_hash, info->share_name);
}

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

static char *
get_string_from_key_file (GKeyFile *key_file, const char *group, const char *key)
{
	GError *error;
	char *str;

	error = NULL;
	str = NULL;

	if (g_key_file_has_key (key_file, group, key, &error)) {
		str = g_key_file_get_string (key_file, group, key, &error);
		if (!str) {
			g_assert (!g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_NOT_FOUND)
				  && !g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND));

			g_error_free (error);
		}
	} else {
		g_assert (!g_error_matches (error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND));
		g_error_free (error);
	}

	return str;
}

static void
add_key_group_to_hashes (GKeyFile *key_file, const char *group)
{
	char *path;
	char *comment;
	char *acl;
	gboolean is_writable;
	ShareInfo *info;

	path = get_string_from_key_file (key_file, group, KEY_PATH);
	if (!path) {
		g_message ("group '%s' doesn't have a '%s' key!  Ignoring group.", group, KEY_PATH);
		return;
	}

	comment = get_string_from_key_file (key_file, group, KEY_COMMENT);

	acl = get_string_from_key_file (key_file, group, KEY_ACL);
	if (acl) {
		if (strcmp (acl, "Everyone:R") == 0)
			is_writable = FALSE;
		else if (strcmp (acl, "Everyone:F") == 0)
			is_writable = TRUE;
		else {
			g_message ("unknown format for key '%s/%s' as it contains '%s'.  Assuming that the share is read-only",
				   group, KEY_ACL, acl);
			is_writable = FALSE;
		}

		g_free (acl);
	} else {
		g_message ("group '%s' doesn't have a '%s' key!  Assuming that the share is read-only.", group, KEY_ACL);
		is_writable = FALSE;
	}

	g_assert (path != NULL);

	if (lookup_share_by_path (path) || lookup_share_by_share_name (group)) {
		g_message ("Got a duplicate share called '%s' or with path '%s'!  Ignoring duplicate.",
			   group, path);
		g_free (path);
		g_free (comment);
		return;
	}

	info = g_new (ShareInfo, 1);
	info->path = path;
	info->share_name = g_strdup (group);
	info->comment = comment;
	info->is_writable = is_writable;

	g_hash_table_insert (path_share_info_hash, info->path, info);
	g_hash_table_insert (share_name_share_info_hash, info->share_name, info);
}

static gboolean
refresh_shares (GError **error)
{
	GKeyFile *key_file;
	char *argv[1];
	gsize num_groups;
	char **group_names;
	int i;
	GError *real_error;

	free_all_shares ();

	if (throw_error_on_refresh) {
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_FAILED,
			     _("Failed"));
		return FALSE;
	}

	argv[0] = "list";

	real_error = NULL;
	if (!net_usershare_run (G_N_ELEMENTS (argv), argv, &key_file, &real_error)) {
		g_message ("Called \"net usershare list\" but it failed: %s", real_error->message);
		g_propagate_error (&error, real_error);
		return FALSE;
	}

	g_assert (key_file != NULL);

	group_names = g_key_file_get_groups (key_file, &num_groups);

	/* FIXME: In add_key_group_to_hashes(), we simply ignore key groups
	 * which have invalid data (i.e. no path).  We could probably accumulate a
	 * GError with the list of invalid groups and propagate it upwards.
	 */
	for (i = 0; i < num_groups; i++) {
		g_assert (group_names[i] != NULL);
		add_key_group_to_hashes (key_file, group_names[i]);
	}

	g_strfreev (group_names);

	return TRUE;
}

static gboolean
refresh_if_needed (GError **error)
{
	gboolean retval;

	if (refresh_timestamp_update_counter == 0) {
		time_t new_timestamp;

		refresh_timestamp_update_counter = NUM_CALLS_BETWEEN_TIMESTAMP_UPDATES;

		new_timestamp = time (NULL);
		if (new_timestamp - refresh_timestamp > TIMESTAMP_THRESHOLD)
			retval = refresh_shares (error);
		else
			retval = TRUE;

		refresh_timestamp = new_timestamp;
	} else {
		refresh_timestamp_update_counter--;
		retval = TRUE;
	}

	return retval;
}

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
	char *argv[5];
	ShareInfo *copy;
	ShareInfo *old_info;
	GError *real_error;

	if (throw_error_on_add) {
		g_set_error (error,
			     SHARES_ERROR,
			     SHARES_ERROR_FAILED,
			     _("Failed"));
		return FALSE;
	}

	argv[0] = "add";
	argv[1] = info->share_name;
	argv[2] = info->path;
	argv[3] = info->comment;
	argv[4] = info->is_writable ? "Everyone:F" : "Everyone:R";

	/* FIXME: the following won't return a keyfile.  Make that assumption in
	 * net_usershare_run() conditional on an argument or something.
	 */

	real_error = NULL;
	if (!net_usershare_run (G_N_ELEMENTS (argv), argv, &key_file, &real_error)) {
		g_message ("Called \"net usershare add\" but it failed: %s", real_error->message);
		g_propagate_error (&error, real_error);
		return FALSE;
	}

	old_info = lookup_share_by_path (info->path);
	if (old_info) {
		remove_share_info_from_hashes (old_info);
		shares_free_share_info (old_info);
	}

	g_assert (lookup_share_by_share_name (info->share_name) == NULL);

	copy = copy_share_info (info);
	add_share_info_to_hashes (copy);

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

	/* FIXME: use net_usershare */

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

	/* FIXME: FAKE: remove from the other hash! */
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
 * @ret_is_shared: Location to store result value (#TRUE if the path is shared, #FALSE otherwise)
 * @error: Location to store error, or #NULL.
 *
 * Checks whether a path is shared through Samba.
 *
 * Return value: #TRUE if the info could be queried successfully, #FALSE
 * otherwise.  If this function returns #FALSE, an error code will be returned in the
 * @error argument, and *@ret_is_shared will be set to #FALSE.
 **/
gboolean
shares_get_path_is_shared (const char *path, gboolean *ret_is_shared, GError **error)
{
	g_assert (ret_is_shared != NULL);
	g_assert (error == NULL || *error == NULL);

	if (!refresh_if_needed (error)) {
		*ret_is_shared = FALSE;
		return FALSE;
	}

	*ret_is_shared = (lookup_share_by_path (path) != NULL);

	return TRUE;
}

/**
 * shares_get_share_info_for_path:
 * @path: A full path name ("/foo/bar/baz") in file system encoding.
 * @ret_share_info: Location to store result with the share's info - on return,
 * will be non-NULL if the path is indeed shared, or #NULL if the path is not
 * shared.  You must free the non-NULL value with shares_free_share_info().
 * @error: Location to store error, or #NULL.
 * 
 * Queries the information for a shared path:  its share name, its read-only status, etc.
 * 
 * Return value: #TRUE if the info could be queried successfully, #FALSE
 * otherwise.  If this function returns #FALSE, an error code will be returned in the
 * @error argument, and *@ret_share_info will be set to #NULL.
 **/
gboolean
shares_get_share_info_for_path (const char *path, ShareInfo **ret_share_info, GError **error)
{
	ShareInfo *info;

	g_assert (path != NULL);
	g_assert (ret_share_info != NULL);
	g_assert (error == NULL || *error == NULL);

	if (!refresh_if_needed (error)) {
		*ret_share_info = NULL;
		return FALSE;
	}

	info = lookup_share_by_path (path);
	*ret_share_info = copy_share_info (info);

	return TRUE;
}

/**
 * shares_get_share_name_exists:
 * @share_name: Name of a share.
 * @ret_exists: Location to store return value; #TRUE if the share name exists, #FALSE otherwise.
 *
 * Queries whether a share name already exists in the user's list of shares.
 * 
 * Return value: #TRUE if the info could be queried successfully, #FALSE
 * otherwise.  If this function returns #FALSE, an error code will be returned in the
 * @error argument, and *@ret_exists will be set to #FALSE.
 **/
gboolean
shares_get_share_name_exists (const char *share_name, gboolean *ret_exists, GError **error)
{
	g_assert (share_name != NULL);
	g_assert (ret_exists != NULL);
	g_assert (error == NULL || *error == NULL);

	if (!refresh_if_needed (error)) {
		*ret_exists = FALSE;
		return FALSE;
	}

	*ret_exists = (lookup_share_by_share_name (share_name) != NULL);

	return TRUE;
}

/**
 * shares_get_share_info_for_share_name:
 * @share_name: Name of a share.
 * @ret_share_info: Location to store result with the share's info - on return,
 * will be non-NULL if there is a share for the specified name, or #NULL if no
 * share has such name.  You must free the non-NULL value with
 * shares_free_share_info().
 * @error: Location to store error, or #NULL.
 * 
 * Queries the information for the share which has a specific name.
 * 
 * Return value: #TRUE if the info could be queried successfully, #FALSE
 * otherwise.  If this function returns #FALSE, an error code will be returned in the
 * @error argument, and *@ret_share_info will be set to #NULL.
 **/
gboolean
shares_get_share_info_for_share_name (const char *share_name, ShareInfo **ret_share_info, GError **error)
{
	ShareInfo *info;

	g_assert (share_name != NULL);
	g_assert (ret_share_info != NULL);
	g_assert (error == NULL || *error == NULL);

	if (!refresh_if_needed (error)) {
		*ret_share_info = NULL;
		return FALSE;
	}

	info = lookup_share_by_share_name (share_name);
	*ret_share_info = copy_share_info (info);

	return TRUE;
}

/**
 * shares_modify_share:
 * @old_path: Path of the share to modify, or %NULL.
 * @info: Info of the share to modify/add, or %NULL to delete a share.
 * @error: Location to store error, or #NULL.
 * 
 * Can add, modify, or delete shares.  To add a share, pass %NULL for @old_path, and a non-null @info.
 * To modify a share, pass a non-null @old_path and non-null @info.  To remove a share, pass a non-NULL
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

	if (!refresh_if_needed (error))
		return FALSE;

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
 * @ret_info_list: Location to store the return value, which is a list
 * of #ShareInfo structures.  Free this with shares_free_share_info_list().
 * @error: Location to store error, or #NULL.
 *
 * Gets the list of shared folders and their information.
 * 
 * Return value: #TRUE if the info could be queried successfully, #FALSE
 * otherwise.  If this function returns #FALSE, an error code will be returned in the
 * @error argument, and *@ret_info_list will be set to #NULL.
 **/
gboolean
shares_get_share_info_list (GSList **ret_info_list, GError **error)
{
	g_assert (ret_info_list != NULL);
	g_assert (error == NULL || *error == NULL);

	if (!refresh_if_needed (error)) {
		*ret_info_list = NULL;
		return FALSE;
	}

	*ret_info_list = NULL;
	g_hash_table_foreach (path_share_info_hash, copy_to_slist_cb, ret_info_list);

	return TRUE;
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
shares_set_debug (gboolean error_on_refresh,
		  gboolean error_on_add,
		  gboolean error_on_modify,
		  gboolean error_on_remove)
{
	throw_error_on_refresh = error_on_refresh;
	throw_error_on_add = error_on_add;
	throw_error_on_modify = error_on_modify;
	throw_error_on_remove = error_on_remove;
}
