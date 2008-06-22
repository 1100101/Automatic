#ifndef AUTOMATIC_H__
#define AUTOMATIC_H__

/*
 * Copyright (C) 2008 Frank Aurich (1100101+automatic@gmail.com)
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

#ifdef MEMWATCH
	#include "memwatch.h"
#endif


#include <stdint.h>
#include "list.h"

struct auto_handle {
	char *feed_url;
	char *log_file;
	char *transmission_path;
	char *statefile;
	linked_list bucket;
	linked_list regex_patterns;
	uint8_t max_bucket_items;
	uint8_t bucket_changed;
	uint8_t check_interval;
	uint8_t use_transmission;
};

typedef struct auto_handle auto_handle;

const char *get_home_folder(void);
void set_home_folder(const char *path);

const char *get_temp_folder(void);

const char* am_getlogfile();
const char* am_get_statefile();
uint8_t am_get_verbose();
uint8_t am_get_nofork();
uint8_t am_get_bucket_size();
void am_set_bucket_size(uint8_t size);
void* am_malloc(size_t size);
void* am_realloc(void *p, size_t size);
void am_free(void *p);

#endif
