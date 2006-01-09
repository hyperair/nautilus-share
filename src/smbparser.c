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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "smbparser.h"

#include <libdaemon/daemon.h>


/*----------------------------------------------------------------------*/
GSList *
add_directive(GSList *shares, const gchar *key, const gchar *value)
{
  Directive *directive;
  Share *s = (Share *)shares->data;

  directive = (Directive *)malloc(sizeof(Directive));
  directive->key = g_strdup(key);
  directive->value = g_strdup(value);
  s->directives = g_slist_append(s->directives,directive);
  return shares;
}

/*----------------------------------------------------------------------*/
GSList *
add_share(GSList *shares, const gchar *sharename, const gchar *username)
{
  Share *share;

  share = (Share *)malloc(sizeof(Share));
  share->sharename= g_strdup(sharename);
  share->username= g_strdup(username);
  share->sharepath= NULL;
  share->directives = NULL;
  return g_slist_append(shares,share);
}

/*----------------------------------------------------------------------*/
void
free_directive(Directive *directive)
{
  g_free(directive->key);
  g_free(directive->value);
  g_free(directive);
}

void
free_share(Share *share)
{
  if(!share)
    return;
  g_free(share->sharename);
  g_free(share->username);
  g_slist_foreach(share->directives,(GFunc)free_directive,NULL);
  g_slist_free(share->directives);
  g_free(share);
}

void
free_shares(GSList *shares)
{
  g_slist_foreach(shares,(GFunc)free_share,NULL);
  g_slist_free(shares);
}
/*----------------------------------------------------------------------*/
gint
cmp_find_sharename(Share *a, char *b)
{
  if (!a || !a->sharename || !b)
    return -1;
  return strcmp(a->sharename, b);
}

GSList *
find_sharename(GSList *shares, gchar *sharename)
{
  if(g_slist_length(shares) == 0 || !sharename)
    return shares;
  return g_slist_find_custom(shares,sharename,(GCompareFunc)cmp_find_sharename);
}

/*----------------------------------------------------------------------*/
gchar *
smbparser_get_section_owner(GSList *shares, gchar *section)
{
  GSList *	tmp;

  if((tmp = find_sharename(shares,section)))
    {
      Share *share=(Share *)tmp->data;
      return share->username;
    }
  return NULL;
}

/*----------------------------------------------------------------------*/
gint
cmp_find_key(Directive *a, char *b)
{
  if (!a || !a->key || !b)
    return -1;
  return strcmp(a->key, b);
}

GSList *
find_key(GSList *directives, gchar *key)
{
  return g_slist_find_custom(directives,key,(GCompareFunc)cmp_find_key);
}

/*----------------------------------------------------------------------*/
/*
 * Ajoute une section dans la liste si elle n'existe pas.
 */
GSList *
smbparser_add_section(GSList *smbparser, gchar *section, gchar *username)
{
  if(!username || (find_sharename(smbparser,section)))
    return smbparser;
  return add_share(smbparser,section, username);
}
/*----------------------------------------------------------------------*/
GSList *
smbparser_remove_section(GSList *smbparser, gchar *section)
{
  GSList *tmp = NULL;
  if((tmp = find_sharename(smbparser,section)))
    {
      smbparser = g_slist_remove_link(smbparser,tmp);
      free_shares(tmp);
    }
  return smbparser;
}


/*----------------------------------------------------------------------*/
GSList *
smbparser_rename_section(GSList *smbparser, gchar *section, gchar *newname)
{
  GSList *tmp = NULL;
  if((tmp = find_sharename(smbparser,section)) && (find_sharename(smbparser,newname) == NULL))
    {
      Share *share=(Share *)tmp->data;
      g_free(share->sharename);
      share->sharename=g_strdup(newname);
    }
  return smbparser;
}


/*----------------------------------------------------------------------*/
gchar *
smbparser_get_key(GSList *smbparser, gchar *section, gchar *key)
{
  GSList *tmp = NULL;
  GSList *tmp_key = NULL;
  Share * tmp_share = NULL;

  if((tmp = find_sharename(smbparser,section)))
    {
      tmp_share = (Share *)tmp->data;
      tmp_key = (GSList *)tmp_share->directives;
      if((tmp_key = find_key(tmp_key, key)))
	{
	  Directive *d = (Directive *)tmp_key->data;
	  return (d->value);
	}
      
    }
  return NULL;
}

/*----------------------------------------------------------------------*/
GSList *
smbparser_set_key(GSList *smbparser, gchar *section, gchar *key, gchar *value)
{
  GSList *tmp = NULL;
  GSList *tmp_key = NULL;
  Directive *d;
  
  if(!section || !key || !value)
    return smbparser;

  if((tmp = find_sharename(smbparser,section)))
    {
      Share * tmp_share = (Share *)tmp->data;
      if((tmp_key = find_key(tmp_share->directives, key)))
	{
	  d = (Directive *)tmp_key->data;
	  g_free(d->value);
	  d->value = g_strdup(value);
	}
      else
	tmp = add_directive(tmp,key,value);
      if(strcmp(key,"path") == 0)
	{
	  tmp_key = find_key(tmp_share->directives,"path");
	  d = (Directive *)tmp_key->data;
	  tmp_share->sharepath = d->value;
	}
    }
  return smbparser;
}
/*----------------------------------------------------------------------*/
GSList *
smbparser_remove_key(GSList *smbparser, gchar *section, gchar *key)
{
  GSList *tmp = NULL;
  GSList *tmp_key = NULL;
  GSList *tmp_keyr= NULL;
  Share * tmp_share = NULL;

  if((tmp = find_sharename(smbparser,section)))
    {
      tmp_share = (Share *)tmp->data;
      tmp_key = (GSList *)tmp_share->directives;
      if((tmp_keyr = find_key(tmp_key, key)))
	{
	  tmp_key = g_slist_remove_link(tmp_key,tmp_keyr);
	  free_directive(tmp_keyr->data);
	  g_slist_free(tmp_keyr);
	  tmp_share->directives = tmp_key;
	}
      
    }
  return smbparser;
}
/*----------------------------------------------------------------------*/
gint
cmp_find_path(Share *a, char *b)
{
  if (!a || !a->sharepath || !b)
    return -1;
  return strcmp(a->sharepath, b);
}

gchar *
smbparser_get_sharename(GSList *smbparser, gchar *path)
{
  GSList *tmp = NULL;
  Share *tmp_s = NULL;

  if((tmp = g_slist_find_custom(smbparser,path,(GCompareFunc)cmp_find_path)))
    {
      tmp_s = (Share *)tmp->data;
      return(tmp_s->sharename);
    }
  return NULL;
}
/*----------------------------------------------------------------------*/
gboolean
smbparser_sharename_is_writable(GSList *smbparser, gchar *path)
{
  char *writable;
  if((writable = smbparser_get_key(smbparser, smbparser_get_sharename(smbparser, path),"writable")))
    if(strcmp(writable,"yes") == 0)
      return TRUE;
  return FALSE;
}
/*----------------------------------------------------------------------*/
void
fdump_directive(Directive *directive, FILE *file)
{
  g_fprintf(file, "\t%s = %s\n", directive->key, directive->value);
}

void
fdump_share(Share *share, FILE *file)
{
  /* FIXME is it really usefull to forbid empty path? */
/*   if (!share || !share->sharepath) */
/*     return; */
  if (!share)
    return;
  g_fprintf(file, "[%s] ; user=\"%s\"\n", share->sharename, share->username);
  g_slist_foreach(share->directives,(GFunc)fdump_directive,file);
  g_fprintf(file, "\n");
}

void
fdump(GSList *shares, FILE *file)
{
  if(!shares)
    return;
  g_slist_foreach(shares,(GFunc)fdump_share,file);
}

void
dump(GSList *shares)
{
  fdump(shares,stderr);
}

void
smbparser_write(GSList *smbparser, const gchar *filename)
{
  FILE *file;
  umask(0022);
  file = fopen(filename,"w+");
  if(!file)
    {
      daemon_log(LOG_ERR, "error writing config file: %s", filename);
      return;
    }
  fdump(smbparser, file);
  fclose(file);
}
/*----------------------------------------------------------------------*/
#define ASCIILINESZ	1024

char * strlwc(char * s)
{
    static char l[ASCIILINESZ+1];
    int i ;

    if (s==NULL) return NULL ;
    memset(l, 0, ASCIILINESZ+1);
    i=0 ;
    while (s[i] && i<ASCIILINESZ) {
        l[i] = (char)tolower((int)s[i]);
        i++ ;
    }
    l[ASCIILINESZ]=(char)0;
    return l ;
}

char * strupc(char * s)
{
    static char l[ASCIILINESZ+1];
    int i ;

    if (s==NULL) return NULL ;
    memset(l, 0, ASCIILINESZ+1);
    i=0 ;
    while (s[i] && i<ASCIILINESZ) {
        l[i] = (char)toupper((int)s[i]);
        i++ ;
    }
    l[ASCIILINESZ]=(char)0;
    return l ;
}

char * strskp(char * s)
{
    char * skip = s;
	if (s==NULL) return NULL ;
    while (isspace((int)*skip) && *skip) skip++;
    return skip ;
} 

char * strcrop(char * s)
{
    static char l[ASCIILINESZ+1];
	char * last ;

    if (s==NULL) return NULL ;
    memset(l, 0, ASCIILINESZ+1);
	strcpy(l, s);
	last = l + strlen(l);
	while (last > l) {
		if (!isspace((int)*(last-1)))
			break ;
		last -- ;
	}
	*last = (char)0;
    return l ;
}

char * strstrip(char * s)
{
    static char l[ASCIILINESZ+1];
	char * last ;
	
    if (s==NULL) return NULL ;
    
	while (isspace((int)*s) && *s) s++;
	
	memset(l, 0, ASCIILINESZ+1);
	strcpy(l, s);
	last = l + strlen(l);
	while (last > l) {
		if (!isspace((int)*(last-1)))
			break ;
		last -- ;
	}
	*last = (char)0;

	return (char*)l ;
}


GSList *
smbparser_load( GSList *smbparser, const gchar *filename)
{
  char        lin[ASCIILINESZ+1];
  char        sec[ASCIILINESZ+1];
  char        username[ASCIILINESZ+1];
  char        key[ASCIILINESZ+1];
  char        val[ASCIILINESZ+1];
  char    *   where ;
  FILE    *   ini ;
  int         lineno ;

  if ((ini=fopen(filename, "r"))==NULL) {
    daemon_log(LOG_ERR,"failed to open file: %s",filename);
    return NULL ;
  }

  sec[0]=0;

  lineno = 0 ;
  while (fgets(lin, ASCIILINESZ, ini)!=NULL) {
    lineno++ ;
    daemon_log(LOG_INFO, "--> %s",lin);
    where = strskp(lin); /* Skip leading spaces */
    if (*where==';' || *where=='#' || *where==0)
      continue ; /* Comment lines */
    else {
      if (sscanf(where, "[%[^]]] ; user=\"%[^\"]\"", sec, username) == 2) {
	/* Valid section name */
	daemon_log(LOG_INFO, "-%s-%s-",sec,username);
 	strcpy(username, strcrop(username));
	
	smbparser = smbparser_add_section(smbparser,sec,username);
      } else if (sscanf (where, "%[^=] = \"%[^\"]\"", key, val) == 2
		 ||  sscanf (where, "%[^=] = '%[^\']'",   key, val) == 2
		 ||  sscanf (where, "%[^=] = %[^;#]",     key, val) == 2) {
/* 	strcpy(key, strlwc(strcrop(key))); */
	strcpy(key, strcrop(key));
	/*
	 * sscanf cannot handle "" or '' as empty value,
	 * this is done here
	 */
	if (!strcmp(val, "\"\"") || !strcmp(val, "''")) {
	  val[0] = (char)0;
	} else {
	  strcpy(val, strcrop(val));
	}
	smbparser = smbparser_set_key(smbparser,sec, key, val);
      }
    }
  }
  fclose(ini);
  return smbparser;
}
/*----------------------------------------------------------------------*/
#ifdef TEST

int
main()
{
  GSList *shares = NULL;

/*   shares = smbparser_add_section(shares,"section1"); */
/*   shares =   smbparser_set_key(shares,"section1","path1","/section11"); */
/*   shares =   smbparser_set_key(shares,"section1","path2","/section12"); */
/*   shares =   smbparser_set_key(shares,"section1","path3","/section13"); */
/*   shares =   smbparser_set_key(shares,"section1","path3","hello world"); */
  
/*   shares =   smbparser_rename_section(shares,"section1","section"); */
/*   shares =   smbparser_remove_section(shares,"section3"); */

/*   shares =   smbparser_add_section(shares,"section4"); */
/*   shares =   smbparser_add_section(shares,"section4"); */
/*   shares =   smbparser_set_key(shares,"section4","path4","/section44"); */
/*   shares =   smbparser_set_key(shares,"section4","path2","/section42"); */
/*   shares =   smbparser_set_key(shares,"section4","path3","/section43"); */

/*   shares =   smbparser_rename_section(shares,"section4","section"); */
 
  shares = smbparser_load(shares,SMB_CONF);
  dump(shares);
/*   printf("-->%s %d\n",smbparser_get_sharename(shares,"/tmp"),  */
/* 	 smbparser_sharename_is_writable(shares,"/tmp")); */
/*   smbparser_write(shares,"smb.conf"); */
/*   free_shares(shares); */
  return(0);
}
#endif
