#ifndef RSS_LIST_H__
#define RSS_LIST_H__

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

typedef struct feed_item *feed_item;


/** A structure to represent an RSS feed item
 */
struct feed_item {
	/** \{ */
	char *name; /**< "Name" field of the RSS item */
	char *url;  /**< URL field of the RSS item    */
	char *category;
	char *guid;
	/** \} */
};

void freeFeedItem(void *item);
feed_item newFeedItem(void);
uint8_t isMatch(const simple_list filters, const char * string, const char * feedID, char **folder,char** filter_pattern);

#endif
