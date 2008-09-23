#ifndef WEB_H__
#define WEB_H__

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
#include "list.h"
#include "automatic.h"

struct HTTPData {
 char *data;
 size_t size;
};

struct WebData {
	char *url;
	long responseCode;
	size_t content_length;
	char *content_filename;
	struct HTTPData *header;
	struct HTTPData *response;
};


typedef struct HTTPData HTTPData;
typedef struct WebData WebData;

WebData* getHTTPData(const char *url);
void WebData_free(struct WebData *data);
void cd_preg_free(void);
void download_torrent(auto_handle *ses, feed_item item);
int uploadTorrent(auto_handle *ah, void *torrent, uint32_t tsize);
#endif
