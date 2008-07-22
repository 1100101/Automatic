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
#include <sys/types.h>
#include <sys/param.h>

#include "output.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

void* am_malloc( size_t size ) {
	void *tmp = NULL;
	if(size > 0) {
		tmp = malloc(size);
		dbg_printf(P_DBG, "Allocated %d bytes (%p)", size, tmp);
		return tmp;
	}
	return NULL;
}

void* am_realloc(void *p, size_t size) {
	if(!p)
		return am_malloc(size);
	else
		return realloc(p, size);
}

void am_free(void * p) {
	if(p) {
		dbg_printf(P_INFO2, "Freeing %p", p);
		free(p);
/* 		p = NULL; */
	}
}

char* am_strdup(char *str) {
	int len;
	char *buf = NULL;
	if(str) {
		len = strlen(str);
		buf = am_malloc(len + 1);
		strncpy(buf, str, len);
		buf[len]= '\0';
	}
	return buf;
}

char* am_strndup(char *str, int len) {
	char *buf = NULL;
	if(str) {
		buf = am_malloc(len + 1);
		strncpy(buf, str, len);
		buf[len]= '\0';
	}
	return buf;
}

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

char* resolve_path(char *path) {
	char new_dir[MAXPATHLEN];
	char *homedir = NULL;

	if(path && strlen(path) > 2) {
		/* home dir */
		if(path[0] == '~' && path[1] == '/') {
			homedir = get_home_folder();
			if(homedir) {
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

char* get_tr_folder() {
	static char *path = NULL;
	char buf[MAXPATHLEN];
	char *home = NULL;

	if(!path) {
		home = get_home_folder();
		strcpy(buf, home);
		strcat(buf, "/.config/transmission");
		path = am_strdup(buf);
		am_free(home);
	}
	return path;
}

char* get_temp_folder() {
	static char *dir = NULL;

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
