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

#define AM_DEFAULT_INTERVAL			30
#define AM_DEFAULT_HOST  				"localhost"
#define AM_DEFAULT_RPCPORT 			9091
#define AM_TRANSMISSION_1_2			2
#define AM_TRANSMISSION_1_3			3

#include <stdint.h>

#include "feed_item.h"
#include "rss_feed.h"

/** \cond */
struct auto_handle {
	char *transmission_path;
	char *statefile;
	char *torrent_folder;
	char *watch_folder;
	char *auth;
	char *host;
	rss_feeds 	feeds;
	simple_list downloads;
	simple_list filters;
	uint8_t 		max_bucket_items;
	uint8_t 		bucket_changed;
	uint8_t 		check_interval;
	uint8_t 		use_transmission;
	uint16_t 		rpc_port;
	uint8_t 		transmission_version;
};
/** \endcond */

typedef struct auto_handle auto_handle;

uint8_t am_get_verbose(void);

#endif
