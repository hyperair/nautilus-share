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

#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <libdaemon/daemon.h>

#include "smbparser-dbus.h"
#include "smbparser-dbus-server.h"
#include "smbparser.h"


#define DELIM "\r\n;[]\"="
extern GSList *shares;

/******************************************************************************/
/* keys that the user is authorized to used */
static char *authorized_keys[] = {
  "comment",
  "path",
  "public",
  "writable",
  NULL};

/* key that a user mustn't used, only root can use them */
static char *forbidden_sections[] = {
  "global",
  "homes",
  "print$",
  "printers",
  "netlogon",
  NULL};

/* check that the key is in the authorized_keys */
gboolean
authorized_key(char *key)
{
  int i;
  /*FIXME unicode */
  for(i = 0; authorized_keys[i] != NULL ; i++)
    if(g_ascii_strcasecmp(key,authorized_keys[i]) == 0)
      return TRUE;
  daemon_log(LOG_WARNING,"Forbidden Key: %s",key);
  return FALSE;
}

/* check if section name is not in the forbidden_sections */
gboolean
authorized_section(char *section)
{
  int i;
  
  for(i = 0; forbidden_sections[i] != NULL ; i++)
    if(g_ascii_strcasecmp(section, forbidden_sections[i]) == 0)
      {
	daemon_log(LOG_WARNING,"Forbidden Sharename: %s",section);
	return FALSE;
      }
  return TRUE;
}

/* return the user id of the user connected to the bus */
uid_t
get_nautilus_userid(DBusConnection  *connection)
{
  uid_t user_id;
  DBusError error;
      
  dbus_error_init (&error);
  user_id = (uid_t)dbus_bus_get_unix_user(connection, NAUTILUS_DBUS_SRV, &error);
  if(user_id < 0)
    {
      daemon_log(LOG_ERR,"error message: %s", error.message);
      dbus_error_free (&error);
      return user_id;
    }
  dbus_error_free (&error);
  return user_id;
}

/* return the username of the user connected to the bus */
char *
get_nautilus_user(DBusConnection  *connection)
{
  struct passwd		*my_passwd;
  uid_t			user_id;
  
  user_id = get_nautilus_userid(connection);
  if((user_id < 0) || !(my_passwd = getpwuid(user_id)))
    return NULL;
  return(my_passwd->pw_name);
}

/* check if the user owns the section */
gboolean
user_owns_section(gchar *username, gchar *section)
{
  char *	owner;
  if((owner = smbparser_get_section_owner(shares,section)))
    if (strcmp(owner,username) == 0)
      return TRUE;
  daemon_log(LOG_WARNING,"User %s doesn't own section %s!", username,section);
  return FALSE;
}

/* check if the user connected to dbus owns the section */
gboolean
nautilus_user_owns_section(DBusConnection  *connection, gchar *section)
{
  gchar *	nuser;

  if((nuser =get_nautilus_user(connection)) && user_owns_section(nuser,section))
    return TRUE;
  return FALSE;
}

/* check that the path belongs to the user who want to share it */
/* or that the user is root */
gboolean
authorized_path(DBusConnection  *connection, char *path)
{
  uid_t		user_id;
  struct stat   statbuf;
  
  if (stat(path, &statbuf) == -1)
    return FALSE;
  
  user_id = get_nautilus_userid(connection);
  /* if user root or path belongs to user we return TRUE */
  if((user_id > 0 && user_id == statbuf.st_uid) 
     || user_id == 0)
    return TRUE;
  else
    {
      daemon_log(LOG_WARNING,"Forbidden path %s used by user %s!", path, get_nautilus_user(connection));
      return FALSE;
    }
}
/******************************************************************************/
void
path_unregistered_func (DBusConnection  *connection,
                        void            *user_data)
{
  /* connection was finalized */
}

/******************************************************************************/
void
add_sharepath_to_iter(Share *share, gpointer user_data)
{
  DBusMessageIter *iter = (DBusMessageIter *)user_data;
#ifndef DBUS_USE_NEW_API
  dbus_message_iter_append_string(iter, share->sharepath);
#else /* DBUS_USE_NEW_API */
  dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &share->sharepath);
#endif /* DBUS_USE_NEW_API */
}
/******************************************************************************/
DBusHandlerResult
path_message_func (DBusConnection  *connection,
                   DBusMessage     *message,
                   void            *user_data)
{
  /***************
   * ShareGetKey *
   ***************/
  if (dbus_message_is_method_call (message, SMBPARSER_DBUS_INTERFACE, "ShareGetKey")) {
    DBusError	error;
    char	*path;
    char	*key;
    char	*value;
    char	*sharename;
    DBusMessage	*reply_message = NULL;
    
    dbus_error_init (&error);
    if (dbus_message_get_args 
       (message, &error, 
	DBUS_TYPE_STRING, &path, 
	DBUS_TYPE_STRING, &key, 
	DBUS_TYPE_INVALID)) {
/*     FIXME: check that the section exists */
      reply_message = dbus_message_new_method_return (message);
      if((sharename = smbparser_get_sharename(shares, path)))
	{
	  value =  smbparser_get_key(shares,sharename,key);
#ifdef DBUS_USE_NEW_API
	  if (!value)
	    value = "";
#endif /* DBUS_USE_NEW_API */
	  daemon_log(LOG_INFO,"Share Get key %s in section %s -> %s", key, sharename,value);
#ifndef DBUS_USE_NEW_API
	  dbus_message_append_args (reply_message, DBUS_TYPE_STRING, value?value:"", DBUS_TYPE_INVALID);
#endif /* ! DBUS_USE_NEW_API */
	}
      else
	{
	  daemon_log(LOG_ERR,"Share Get key path: %s not found",path);
#ifndef DBUS_USE_NEW_API
	  dbus_message_append_args (reply_message, DBUS_TYPE_STRING, "", DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
	  value = "";
#endif /* DBUS_USE_NEW_API */
	}
#ifndef DBUS_USE_NEW_API
      dbus_free (path);
      dbus_free (key);
#else /* DBUS_USE_NEW_API */
      dbus_message_append_args (reply_message, DBUS_TYPE_STRING, &value, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
      if (reply_message)
	{
	  dbus_connection_send (connection, reply_message, NULL);
	  dbus_message_unref (reply_message);
	}
    } else {
      daemon_log(LOG_ERR,"GetKey received, but error getting message: %s", error.message);
      dbus_error_free (&error);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  else
    /****************
     * ShareGetName *
     ****************/
  if (dbus_message_is_method_call (message, SMBPARSER_DBUS_INTERFACE, "ShareGetName")) {
    DBusError error;
    char *path;
    char *sharename;
    DBusMessage		*reply_message = NULL;
    
    dbus_error_init (&error);
    if (dbus_message_get_args 
       (message, &error, 
	DBUS_TYPE_STRING, &path, 
	DBUS_TYPE_INVALID)) {
      reply_message = dbus_message_new_method_return (message);
      if (!(sharename =  smbparser_get_sharename(shares, path)))
	{
	  daemon_log(LOG_ERR,"SGN Sharename path:%s not found", path);
#ifndef DBUS_USE_NEW_API
	  dbus_message_append_args (reply_message, DBUS_TYPE_STRING, "", DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
	  sharename = "";
#endif /* DBUS_USE_NEW_API */
	}
      else
	{
	  daemon_log(LOG_INFO,"Path %s is sharename %s", path, sharename);
#ifndef DBUS_USE_NEW_API
	  dbus_message_append_args (reply_message, DBUS_TYPE_STRING, sharename, DBUS_TYPE_INVALID);
#endif /* ! DBUS_USE_NEW_API */
	}
#ifndef DBUS_USE_NEW_API
      dbus_free (path);
#else /* DBUS_USE_NEW_API */
      dbus_message_append_args (reply_message, DBUS_TYPE_STRING, &sharename, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
      if (reply_message)
	{
	  dbus_connection_send (connection, reply_message, NULL);
	  dbus_message_unref (reply_message);
	}
    } else {
      daemon_log(LOG_ERR,"GetSharename received, but error getting message: %s", error.message);
      dbus_error_free (&error);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  else
  /*****************
   * ShareRename *
   *****************/
   if (dbus_message_is_method_call (message, SMBPARSER_DBUS_INTERFACE, "ShareRename")) {
    DBusError error;
    char *path;
    char *sharename;
    char *newname;
    DBusMessage	*reply_message = NULL;
#ifdef DBUS_USE_NEW_API
    dbus_uint32_t reply_val;
#endif /* DBUS_USE_NEW_API */

    dbus_error_init (&error);
    if (dbus_message_get_args 
       (message, &error, 
	DBUS_TYPE_STRING, &path, 
	DBUS_TYPE_STRING, &newname, 
	DBUS_TYPE_INVALID)) {
      reply_message = dbus_message_new_method_return (message);
      g_strdelimit(newname,DELIM,'_');

      if (!(sharename = smbparser_get_sharename(shares, path)))
	{
#ifdef DBUS_USE_NEW_API
	  reply_val = 2;
#endif /* DBUS_USE_NEW_API */
	  daemon_log(LOG_ERR,"ShareRename Sharename path:%s not found", path);
#ifndef DBUS_USE_NEW_API
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 2, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
	}
      else
	{
	  daemon_log(LOG_INFO,"ShareRename Renaming share %s in section %s with path %s", sharename,newname,path);
	  if(nautilus_user_owns_section(connection, sharename))
	    {
	      /* FIXME strlen(sharename) et meme modele que ShareAdd */
	      if(authorized_section(sharename) == FALSE)
		{
#ifdef DBUS_USE_NEW_API
		  reply_val = 1;
#endif /* DBUS_USE_NEW_API */
		  daemon_log(LOG_ERR,"ShareRename received, but error: authorized_section");
#ifndef DBUS_USE_NEW_API
		  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 1, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
		  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
		}
	      else
		if(find_sharename(shares,newname))
		  {
#ifdef DBUS_USE_NEW_API
		    reply_val = 1;
#endif /* DBUS_USE_NEW_API */
		    daemon_log(LOG_ERR,"ShareRename Sharename already in used: %s", newname);
#ifndef DBUS_USE_NEW_API
		    dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 1, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
		    dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
		  }
		else
		  {
#ifdef DBUS_USE_NEW_API
		    reply_val = 0;
#endif /* DBUS_USE_NEW_API */
		    daemon_log(LOG_ERR,"ShareRename Sharename not in used: %s", newname);
		    shares =   smbparser_rename_section(shares,sharename,newname);
#ifndef DBUS_USE_NEW_API
		    dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 0, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
		    dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
		  }
	    }
	}
#ifndef DBUS_USE_NEW_API
      dbus_free (path);
      dbus_free (newname);
#endif /* ! DBUS_USE_NEW_API */
      if (reply_message)
	{
	  dbus_connection_send (connection, reply_message, NULL);
	  dbus_message_unref (reply_message);
	}
    } else {
      daemon_log(LOG_ERR,"RenameSection received, but error getting message: %s", error.message);
      dbus_error_free (&error);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  else 
    /**************
     * ShareAdd   *
     **************/
    if (dbus_message_is_method_call (message, SMBPARSER_DBUS_INTERFACE, "ShareAdd")) {
      DBusError	error;
      char	*path;
      char	*name;
      char	*username;
      DBusMessage	*reply_message = NULL;
#ifdef DBUS_USE_NEW_API
      dbus_uint32_t reply_val;
#endif /* DBUS_USE_NEW_API */

      dbus_error_init (&error);
      if (!dbus_message_get_args
	  (message, &error, 
	   DBUS_TYPE_STRING, &path, 
	   DBUS_TYPE_STRING, &name, 
	   DBUS_TYPE_INVALID)) 
	{
	  daemon_log(LOG_ERR,"ShareAdd received, but error: %s", error.message);
	  dbus_error_free (&error);
	  return DBUS_HANDLER_RESULT_HANDLED; 
	}
      g_strdelimit(name,DELIM,'_');
      daemon_log(LOG_INFO,"Adding Share: %s with path %s", name, path);
      reply_message = dbus_message_new_method_return (message);
      if (name && (strlen(name) == 0))
	{
#ifdef DBUS_USE_NEW_API
	  reply_val = 4;
#endif /* DBUS_USE_NEW_API */
	  daemon_log(LOG_ERR,"ShareAdd received, but error: strlen(name) == 0");
#ifndef DBUS_USE_NEW_API
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 4, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
	  goto exit;
	}
      if (!authorized_section(name))
	{
#ifdef DBUS_USE_NEW_API
	  reply_val = 3;
#endif /* DBUS_USE_NEW_API */
	  daemon_log(LOG_ERR,"ShareAdd received, but error: authorized_section");
#ifndef DBUS_USE_NEW_API
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 3, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
	  goto exit;
	}
      if (!authorized_path(connection, path))
	{
#ifdef DBUS_USE_NEW_API
	  reply_val = 2;
#endif /* DBUS_USE_NEW_API */
	  daemon_log(LOG_ERR,"ShareAdd received, but error: authorized_path");
#ifndef DBUS_USE_NEW_API
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 2, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
	  goto exit;
	}
      if(find_sharename(shares,name))
	{
#ifdef DBUS_USE_NEW_API
	  reply_val = 1;
#endif /* DBUS_USE_NEW_API */
	  daemon_log(LOG_ERR,"ShareAdd Name already in used: %s", name);
#ifndef DBUS_USE_NEW_API
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 1, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
	  goto exit;
	}
      if(smbparser_get_sharename(shares, path))
	{
#ifdef DBUS_USE_NEW_API
	  reply_val = 3;
#endif /* DBUS_USE_NEW_API */
	  daemon_log(LOG_ERR,"ShareAdd Path already in used: %s", path);
#ifndef DBUS_USE_NEW_API
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 3, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
	  goto exit;
	}
      if((username = get_nautilus_user(connection)))
	{
#ifdef DBUS_USE_NEW_API
	  reply_val = 0;
#endif /* DBUS_USE_NEW_API */
	  daemon_log(LOG_INFO,"ShareAdd Name not in used: %s", name);
	  shares = smbparser_add_section(shares,name,username);
	  shares = smbparser_set_key(shares,name,"force user", username);
	  shares = smbparser_set_key(shares,name,"path",path);
#ifndef DBUS_USE_NEW_API
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 0, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
	}
    exit:
#ifndef DBUS_USE_NEW_API
      dbus_free (name);
      dbus_free (path);
#endif /* ! DBUS_USE_NEW_API */
      if (reply_message)
	{
	  dbus_connection_send (connection, reply_message, NULL);
	  dbus_message_unref (reply_message);
	}
      dbus_error_free (&error);
      return DBUS_HANDLER_RESULT_HANDLED; 
    }      
      
  /***************
   * SectionUsed *
   ***************/
  else if (dbus_message_is_method_call (message, SMBPARSER_DBUS_INTERFACE, "SectionUsed")) {
    DBusError	error;
    char	*sharename;
    DBusMessage	*reply_message = NULL;
#ifdef DBUS_USE_NEW_API
    dbus_uint32_t reply_val;
#endif /* DBUS_USE_NEW_API */

    dbus_error_init (&error);
    if (dbus_message_get_args 
	(message, &error, DBUS_TYPE_STRING, &sharename, DBUS_TYPE_INVALID)) {
      daemon_log(LOG_INFO,"SectionUsed?: %s", sharename);
      /* keeping only the 12 first char */
/*       sharename[12]=0; */
      reply_message = dbus_message_new_method_return (message);
      
      if(authorized_section(sharename) == FALSE)
	{
#ifdef DBUS_USE_NEW_API
	  reply_val = 1;
#endif /* DBUS_USE_NEW_API */
	  daemon_log(LOG_ERR,"SectionUsed received, but error: authorized_section");
#ifndef DBUS_USE_NEW_API
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 1, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
	  dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
	}
      else
	if(find_sharename(shares,sharename))
	  {
#ifdef DBUS_USE_NEW_API
	    reply_val = 1;
#endif /* DBUS_USE_NEW_API */
	    daemon_log(LOG_ERR,"SU Sharename already in used: %s", sharename);
#ifndef DBUS_USE_NEW_API
	    dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 1, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
	    dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
	  }
	else
	  {
#ifdef DBUS_USE_NEW_API
	    reply_val = 0;
#endif /* DBUS_USE_NEW_API */
	    daemon_log(LOG_INFO,"SU Sharename not in used: %s", sharename);
#ifndef DBUS_USE_NEW_API
	    dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, 0, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
	    dbus_message_append_args (reply_message, DBUS_TYPE_UINT32, &reply_val, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
	  }
#ifndef DBUS_USE_NEW_API
      dbus_free (sharename);
#endif /* ! DBUS_USE_NEW_API */
      if (reply_message)
	{
	  dbus_connection_send (connection, reply_message, NULL);
	  dbus_message_unref (reply_message);
	}
    } else {
      daemon_log(LOG_ERR,"SectionUsed received, but error getting message: %s", error.message);
      dbus_error_free (&error);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  /*****************
   * GetSharepaths *
   *****************/
  else if (dbus_message_is_method_call (message, SMBPARSER_DBUS_INTERFACE, "GetSharepaths")) {
    DBusMessage	*reply_message = NULL;
    DBusMessageIter iter;
    char *empty_string = "";

    reply_message = dbus_message_new_method_return (message);

    dbus_message_iter_init(reply_message, &iter);
    g_slist_foreach(shares,(GFunc)add_sharepath_to_iter,&iter);
#ifndef DBUS_USE_NEW_API
    dbus_message_iter_append_string(&iter, empty_string);
#else /* DBUS_USE_NEW_API */
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &empty_string);
#endif /* DBUS_USE_NEW_API */

    if (reply_message)
      {
	dbus_connection_send (connection, reply_message, NULL);
	dbus_message_unref (reply_message);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  /***********************
   * ShareIsWritable *
   ***********************/
  else if (dbus_message_is_method_call (message, SMBPARSER_DBUS_INTERFACE, "ShareIsWritable")) {
    DBusError error;
    gboolean writable;
    char *path;
    DBusMessage		*reply_message = NULL;
    
    dbus_error_init (&error);
    if (dbus_message_get_args 
       (message, &error, 
	DBUS_TYPE_STRING, &path, 
	DBUS_TYPE_INVALID)) {
      writable =  smbparser_sharename_is_writable(shares,path);
      daemon_log(LOG_INFO,"Path %s -> %d", path, writable);
#ifndef DBUS_USE_NEW_API
      dbus_free (path);
#endif /* ! DBUS_USE_NEW_API */
      reply_message = dbus_message_new_method_return (message);
#ifndef DBUS_USE_NEW_API
      dbus_message_append_args (reply_message, DBUS_TYPE_BOOLEAN, writable, DBUS_TYPE_INVALID);
#else /* DBUS_USE_NEW_API */
      dbus_message_append_args (reply_message, DBUS_TYPE_BOOLEAN, &writable, DBUS_TYPE_INVALID);
#endif /* DBUS_USE_NEW_API */
      if (reply_message)
	{
	  dbus_connection_send (connection, reply_message, NULL);
	  dbus_message_unref (reply_message);
	}
    } else {
      daemon_log(LOG_ERR,"ShareIsWritable received, but error getting message: %s", error.message);
      dbus_error_free (&error);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/******************************************************************************/
DBusHandlerResult
signal_filter (DBusConnection	*connection, 
	       DBusMessage	*message, 
	       void		*user_data)
{
  GMainLoop *loop = user_data;

#ifndef DBUS_USE_NEW_API
  if (dbus_message_is_signal(message, DBUS_INTERFACE_ORG_FREEDESKTOP_LOCAL, "Disconnected")) {
#else /* DBUS_USE_NEW_API */
  if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected")) {
#endif /* DBUS_USE_NEW_API */
    g_main_loop_quit (loop);
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  /*************
   * WriteConf *
   *************/
  else if (dbus_message_is_signal (message, SMBPARSER_DBUS_INTERFACE, "WriteConf")) {
    smbparser_write(shares, SMB_CONF);
    daemon_log(LOG_INFO,"Writing Config.");
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  /********
   * Dump *
   ********/
  else if (dbus_message_is_signal (message, SMBPARSER_DBUS_INTERFACE, "Dump")) {
    dump(shares);
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  /***************
   * ShareSetKey *
   ***************/
  else if (dbus_message_is_signal (message, SMBPARSER_DBUS_INTERFACE, "ShareSetKey")) {
    DBusError error;
    char *sharename;
    char *path;
    char *key;
    char *value;

    dbus_error_init (&error);
    if (dbus_message_get_args 
       (message, &error, 
	DBUS_TYPE_STRING, &path, 
	DBUS_TYPE_STRING, &key, 
	DBUS_TYPE_STRING, &value, 
	DBUS_TYPE_INVALID)) 
      {
	g_strdelimit(key,DELIM,'_');
	g_strdelimit(value,DELIM,'_');
	if((sharename = smbparser_get_sharename(shares, path)))
	  {
	    daemon_log(LOG_INFO,"Share Setting key: %s=%s in section %s", key,value,sharename);
	    
	    if(nautilus_user_owns_section(connection, sharename))
	      if(authorized_key(key))
		shares = smbparser_set_key(shares,sharename,key,value);
	  }
	else
	  daemon_log(LOG_ERR,"SSK Sharename path:%s not found", path);
	
#ifndef DBUS_USE_NEW_API
	dbus_free (path);
	dbus_free (key);
	dbus_free (value);
#endif /* ! DBUS_USE_NEW_API */
      } else {
	daemon_log(LOG_ERR,"SetKey received, but error getting message: %s", error.message);
	dbus_error_free (&error);
      }
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  /******************
   * ShareRemoveKey *
   ******************/
  else if (dbus_message_is_signal (message, SMBPARSER_DBUS_INTERFACE, "ShareRemoveKey")) {
    DBusError error;
    char *sharename;
    char *path;
    char *key;

    dbus_error_init (&error);
    if (dbus_message_get_args 
       (message, &error, 
	DBUS_TYPE_STRING, &path, 
	DBUS_TYPE_STRING, &key, 
	DBUS_TYPE_INVALID)) {
      if((sharename = smbparser_get_sharename(shares, path)))
	{
	  daemon_log(LOG_INFO,"Removing key: %s in section %s", key,sharename);
	  if(nautilus_user_owns_section(connection, sharename))
	    shares =   smbparser_remove_key(shares,sharename,key);
	}
      else
	daemon_log(LOG_ERR,"SRK Sharename path:%s not found", path);

#ifndef DBUS_USE_NEW_API
      dbus_free (path);
      dbus_free (key);
#endif /* ! DBUS_USE_NEW_API */
    } else {
      daemon_log(LOG_ERR,"RemoveKey received, but error getting message: %s", error.message);
      dbus_error_free (&error);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  /*****************
   * ShareRemove   *
   *****************/
  else if (dbus_message_is_signal (message, SMBPARSER_DBUS_INTERFACE, "ShareRemove")) {
    DBusError error;
    char *name;
    char *path;
      
    dbus_error_init (&error);
    if (dbus_message_get_args 
       (message, &error, 
	DBUS_TYPE_STRING, &path, 
	DBUS_TYPE_INVALID)) {
      if((name = smbparser_get_sharename(shares, path)))
	{
	  daemon_log(LOG_INFO,"Removing Share %s", name);
	  if(nautilus_user_owns_section(connection, name))
	    shares =   smbparser_remove_section(shares, name);
	}
      else
	daemon_log(LOG_ERR,"SRS Sharename path:%s not found", path);
#ifndef DBUS_USE_NEW_API
      dbus_free (path);
#endif /* ! DBUS_USE_NEW_API */
    } else {
      daemon_log(LOG_ERR,"RemoveSection received, but error getting message: %s", error.message);
      dbus_error_free (&error);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
