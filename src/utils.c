/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file utils.c
 *
 * Several useful functions to make coding easier.
 */

/*
 * Copyright (C) 2008 Frank Aurich (1100101+automatic@gmail.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <assert.h>

#include "output.h"

#ifdef MEMWATCH
  #include "memwatch.h"
#endif

/** \brief allocate memory on the heap
 *
 * \param size Number of bytes to be allocated
 * \return A pointer to the newly allocated memory, or NULL in case of a failure.
 */
void* am_malloc( size_t size ) {
  void *tmp = NULL;
  if(size > 0) {
    tmp = malloc(size);
    if(tmp) {
      dbg_printf(P_MEM, "Allocated %d bytes (%p)", size, tmp);
    }
    return tmp;
  }
  return NULL;
}

/** \brief Reallocate (resize) a memory section
 *
 * \param p Pointer to existing mem allocation
 * \param size new size of the memory section
 * \return Pointer to the newly allocated memory
 *
 * If p is NULL, new memory is allocated via am_malloc.
 * This is because some implementations of realloc() crash if p is NULL.
 */
void* am_realloc(void *p, size_t size) {
  if(!p) {
    return am_malloc(size);
  }
  dbg_printf(P_MEM, "Reallocating %p to %d bytes", p, size);
  return realloc(p, size);
}

/** \brief Free allocated memory
 *
 * \param p Pointer to allocated memory
 *
 * The function frees the memory pointed to by p. The function does nothing if p is NULL.
 */
void am_free(void * p) {
  if(p) {
    dbg_printf(P_MEM, "Freeing %p", p);
    free(p);
/*     p = NULL; */
  }
}

/** \brief create a copy of a string
 *
 * \param str Pointer to a string
 * \param len length of the given string
 * \return Pointer to a copy of the given string
 *
 * Creates a copy of the given string, but only with a maximum size of len
 */
char* am_strndup(const char *str, int len) {
  char *buf = NULL;
  if(str && *str) {
    buf = am_malloc(len + 1);
    memcpy(buf, str, len);
    buf[len]= '\0';
  }
  return buf;
}

/** \brief create a copy of a string
 *
 * \param str Pointer to a string
 * \return Pointer to a copy of the given string
 */
char* am_strdup(const char *str) {
  if(str && *str) {
    return am_strndup(str, strlen(str));
  }
  return NULL;
/*  int len;
  char *buf = NULL;
  if(str) {
    len = strlen(str);
    buf = am_malloc(len + 1);
    strncpy(buf, str, len);
    buf[len]= '\0';
  }
  return buf;*/
}

/** \brief get the path to the current user's home filder
 *
 * \return Path to the users home folder.
 */
char* get_home_folder() {
  char * dir = NULL;
  struct passwd * pw = NULL;

  if(!dir) {
    if(getenv("HOME")) {
      dir = am_strdup(getenv( "HOME" ));
    } else {
      if((pw = getpwuid(getuid())) != NULL) {
        dir = am_strdup(pw->pw_dir);
        endpwent();
      } else {
        dir = am_strdup("");
      }
    }
  }
  return dir;
}

/** \brief Check a given path for variables and resolve them
 *
 * \param path The given folder or file path
 * \return The resolved path.
 *
 * The function currently only replaces the '~' in paths with the user's home folder.
 */
char* resolve_path(const char *path) {
  char new_dir[MAXPATHLEN];
  char *homedir = NULL;

  if(path && strlen(path) > 2) {
    /* home dir */
    if(path[0] == '~' && path[1] == '/') {
      homedir = get_home_folder();
      if(homedir) {
        assert((strlen(homedir) + strlen(path)) < MAXPATHLEN);
        strcpy(new_dir, homedir);
        strcat(new_dir, ++path);
        am_free(homedir);
        return am_strdup(new_dir);
      }
    }
    return am_strdup(path);
  }
  return NULL;
}

/** \brief Get path to Transmissions default configuration directory
 *
 * \return Path to Transmissions default config folder.
 */
char* get_tr_folder() {
  char *path = NULL;
  char buf[MAXPATHLEN];
  char *home = NULL;

  if(!path) {
    home = get_home_folder();
    assert((strlen(home) + strlen(path)) < MAXPATHLEN);
    strcpy(buf, home);
    strcat(buf, "/.config/transmission");
    path = am_strdup(buf);
    am_free(home);
  }
  return path;
}

/** \brief Get the current user's tempory folder
 *
 * \return Path to the temporary folder.
 */
char* get_temp_folder(void) {
  char *dir = NULL;

  if(!dir) {
    if(getenv("TEMPDIR")) {
      dir = am_strdup(getenv( "TEMPDIR" ));
    } else if(getenv("TMPDIR")) {
      dir = am_strdup(getenv( "TMPDIR" ));
    } else {
      dir = am_strdup("/tmp");
    }
  }
  return dir;
}

/* Function strstrip() lazily obtained from the Transmission project,
** licensed unter the GPL version 2. http://www.transmissionbt.com
*/

char* am_strstrip( char * str ) {
  if( str && *str )
  {
    size_t pos;
    size_t len = strlen( str );

    while( len && isspace( str[len - 1] ) ) {
      --len;
    }

    for( pos = 0; pos < len && isspace( str[pos] ); ) {
      ++pos;
    }

    len -= pos;
    memmove( str, str + pos, len );
    str[len] = '\0';
  }

  return str;
}

char* am_stringToLower(char *string) {
   int i;
   int len = strlen(string);
   
   for(i=0; i<len; i++) {
     if(string[i] >= 'A' && string[i] <= 'Z') {
       string[i] += 32;
     }
   }
   
   return string;
}

char *am_replace_str(const char *s, const char *pattern, const char *subst)
{
  char *ret;
  int i, count = 0;
  size_t substlen, patternlen;
  
  // No replacement possible if either the original string or the pattern is empty
  if(s == NULL || *s == '\0' || pattern == NULL || *pattern == '\0' || subst == NULL ) {
    return NULL;
  }  
   
  substlen = strlen(subst);
  patternlen = strlen(pattern);  

  // No replacement possible if the pattern is longer than the string.
  if(patternlen > strlen(s)) {
    return am_strdup(s);
  }

  // No replacement possible if the pattern is not in the string
  if(strstr(s, pattern) == NULL) {
    return am_strdup(s);
  }

  for (i = 0; s[i] != '\0'; i++) {
    if (strstr(&s[i], pattern) == &s[i]) {
      count++;
      i += patternlen - 1;
    }
  }

  ret = am_malloc(i + 1 + count * (substlen - patternlen));

  if (ret == NULL) {
    return NULL;
  }

  i = 0;
  while (*s) {
    if (strstr(s, pattern) == s) {
      strcpy(&ret[i], subst);
      i += substlen;
      s += patternlen;
    } else {
      ret[i++] = *s++;
    }
  }
  
  ret[i] = '\0';

  return ret;
}
