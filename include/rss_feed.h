#ifndef RSS_FEED_H__
#define RSS_FEED_H__

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

#include "list.h"
#include "utils.h"

typedef struct rss_feed* rss_feed;
typedef struct NODE* rss_feeds;


/** struct representing an RSS feed */
struct rss_feed {
	/** \{ */
	char    *url;  /**< Feed URL */
  char    *cookie;
	uint32_t ttl;	 /**< Time-To-Live for the specific feed */
	/* int32_t count;*/ /**< Item count? (UNUSED) */
	/** \{ */
};

PUBLIC rss_feed feed_new(void);
PUBLIC void feed_free(void* listItem);
PUBLIC void feed_printList(simple_list list);
PUBLIC void feed_add(rss_feed p, NODE **head);

#endif
