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

#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <libdaemon/daemon.h>

#include "smbparser-dbus-server.h"
#include "smbparser-dbus.h"
#include "smbparser.h"

GSList		*shares;
GMainLoop	*loop;

static gboolean foreground = FALSE;
static gboolean killd = FALSE;

static GOptionEntry entries[] = 
  {
    { "foreground", 'f', 0, G_OPTION_ARG_NONE, &foreground, "Don't fork", NULL },
    { "kill", 'k', 0, G_OPTION_ARG_NONE, &killd, "Kill the deamon", NULL },
    { NULL }
  };

/*----------------------------------------------------------------------*/
static DBusObjectPathVTable 
smbparser_vtable = {
  path_unregistered_func,
  path_message_func,
  NULL,
};

/*----------------------------------------------------------------------*/
void
monit_directory(Share *share)
{
/*   We could use this also: */
/*   if(!g_file_test(share->sharepath,G_FILE_TEST_EXISTS)) */

  if(access(share->sharepath,F_OK & S_IFDIR) < 0)
    {
      if (errno == ENOENT)
	{
	  gchar *sharename;
	  daemon_log(LOG_INFO, "Path %s doesn't exist, we unshare it.",share->sharepath);
	  sharename = smbparser_get_sharename(shares,share->sharepath);
	  shares = smbparser_remove_section(shares,sharename);
	  smbparser_write(shares,SMB_CONF);
	}
      else
	daemon_log(LOG_INFO, "Error %d on Path %s",errno, share->sharepath);
    }
}

/*----------------------------------------------------------------------*/
gboolean
check_deleted_folders(gpointer data)
{
  g_slist_foreach(shares,(GFunc)monit_directory,NULL);
  return TRUE;
}

/*----------------------------------------------------------------------*/
int
smbshared ()
{
  DBusConnection	*bus;
  DBusError		error;
  shares = NULL;


  /* Loading .smb.conf */

  if(!(shares = smbparser_load(shares,SMB_CONF)))
    daemon_log(LOG_ERR, "no shares found in %s",SMB_CONF);

  /* Dbus stuff */
  loop = g_main_loop_new (NULL, FALSE);

  dbus_error_init (&error);
  bus = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
  if (!bus) {
    g_warning ("Failed to connect to the D-BUS daemon: %s", error.message);
    dbus_error_free (&error);
    return 1;
  }
  dbus_connection_setup_with_g_main (bus, NULL);

  /* listening to messages from all objects as no path is specified */
  dbus_bus_add_match (bus, 
		      "type='signal',"
		      "interface='" SMBPARSER_DBUS_INTERFACE  "'",
		      &error);
  dbus_connection_add_filter (bus, signal_filter, loop, NULL);

  dbus_connection_register_object_path(bus,SMBPARSER_DBUS_PATH,
				       &smbparser_vtable,
				       NULL);
#ifndef DBUS_USE_NEW_API
  dbus_bus_acquire_service (bus, SMBPARSER_DBUS_SRV, 0, &error);
#else /* DBUS_USE_NEW_API */
  dbus_bus_request_name (bus, SMBPARSER_DBUS_SRV, 0, &error);
#endif /* DBUS_USE_NEW_API */

  /* cleaning non existant folders */
  g_slist_foreach(shares,(GFunc)monit_directory,NULL);
  g_timeout_add(5 * 1000, (GSourceFunc)check_deleted_folders, NULL);
  g_main_loop_run (loop);

  dbus_error_free (&error);
  free_shares(shares);
  return 0;
}

/*----------------------------------------------------------------------*/
void
exit_smbshared()
{
  g_main_loop_quit(loop);
}

/*----------------------------------------------------------------------*/
int
init_daemon(int argc, char **argv)
{
  pid_t pid;

  /* Set indetification string for the daemon for both syslog and PID file */
  daemon_pid_file_ident = daemon_log_ident = daemon_ident_from_argv0(argv[0]);

  /* Check if we are called with -k parameter */
  if (killd == TRUE) {
    int ret;

    /* Kill daemon with SIGINT */
        
    /* Check if the new function daemon_pid_file_kill_wait() is available, if it is, use it. */

/*     if ((ret = daemon_pid_file_kill_wait(SIGINT, 5)) < 0) */
    if ((ret = daemon_pid_file_kill(SIGINT)) < 0)
      daemon_log(LOG_WARNING, "Failed to kill daemon");
        
    return ret < 0 ? 1 : 0;
  }

  /* Check that the daemon is not rung twice a the same time */
  if ((pid = daemon_pid_file_is_running()) >= 0) {
    daemon_log(LOG_ERR, "Daemon already running on PID file %u", pid);
    return 1;
        
  }
  /* Prepare for return value passing from the initialization procedure of the daemon process */
  daemon_retval_init();

  if (foreground == FALSE)
    {
      /* Do the fork */
      if ((pid = daemon_fork()) < 0) {
	
	/* Exit on error */
	daemon_retval_done();
	return 1;
        
      } else if (pid) { /* The parent */
	int ret;
	
	/* Wait for 20 seconds for the return value passed from the daemon process */
	if ((ret = daemon_retval_wait(20)) < 0) {
	  daemon_log(LOG_ERR, "Could not recieve return value from daemon process.");
	  return 255;
	}
	
	if(ret != 0)
	  {
	    if (ret == 1)
	      daemon_log(LOG_ERR, "Could not create PID file.", ret);
	    else
	      daemon_log(LOG_ERR, "Daemon returned %i as return value.", ret);
	  }
	return ret;
        
      } else { /* The daemon */
	/* Create the PID file */
	if (daemon_pid_file_create() < 0) {
	  daemon_log(LOG_ERR, "Could not create PID file (%s).", strerror(errno));
	  
	  /* Send the error condition to the parent process */
	  daemon_retval_send(1);
	  goto finish;
	}
      }
      
      /* Send OK to parent process */
      daemon_retval_send(0);
      
      daemon_log(LOG_INFO, "Sucessfully started");
      signal(SIGINT,exit_smbshared);
      smbshared();
    }
  else
    {
/*       daemon_log_use = DAEMON_LOG_STDOUT; */
/*       if (daemon_pid_file_create() < 0) */
/* 	daemon_log(LOG_ERR, "Could not create PID file (%s).", strerror(errno)); */
/*       else */
	smbshared();
    }
  
  /* Do a cleanup */
 finish:
  daemon_log(LOG_INFO, "Exiting...");
  daemon_pid_file_remove();
  
  return 0;
}

/*----------------------------------------------------------------------*/
int
main (int argc, char **argv)
{
  /* basic deamon stuff */
  GError *error = NULL;
  GOptionContext *context;

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_parse (context, &argc, &argv, &error);

  return init_daemon(argc,argv);
}
