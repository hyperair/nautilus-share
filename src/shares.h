#ifndef SHARES_H
#define SHARES_H

#include <glib.h>

typedef struct {
	char *path;
	char *share_name;
	char *comment;
	gboolean is_writable;
} ShareInfo;

#define SHARES_ERROR (shares_error_quark ())

typedef enum {
	SHARES_ERROR_FAILED,
	SHARES_ERROR_NONEXISTENT
} SharesError;

GQuark shares_error_quark (void);

void shares_free_share_info (ShareInfo *info);

gboolean shares_get_path_is_shared (const char *path);

ShareInfo *shares_get_share_info_for_path (const char *path);

gboolean shares_get_share_name_exists (const char *share_name);

ShareInfo *shares_get_share_info_for_share_name (const char *share_name);

gboolean shares_modify_share (const char *old_path, ShareInfo *info, GError **error);

GSList *shares_get_share_info_list (void);

void shares_free_share_info_list (GSList *list);

void shares_set_debug (gboolean error_on_add,
		       gboolean error_on_modify,
		       gboolean error_on_remove);

#endif
