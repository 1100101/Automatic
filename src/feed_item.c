/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file feed_item.c
 *
 * RSS Item-specific functions.
 */

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

#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>


#include "feed_item.h"
#include "output.h"
#include "regex.h"
#include "utils.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif


/** \brief Check if the provided rss item is a match for any of the given filters
 *
 * \param[in] filters List of regular expressions to check against a given feed item
 * \param[in] item A feed item.
 * \return 1 if a filter matched, 0 otherwise.
 *
 */
uint8_t isMatch(const simple_list filters, const char* string) {
	char *regex_str = NULL;
	simple_list current_regex = NULL;

	current_regex = filters;
	while (current_regex != NULL && current_regex->data != NULL) {
		regex_str = (char*) current_regex->data;
		if(isRegExMatch(regex_str, string) == 1) {
			return 1;
		}
		current_regex = current_regex->next;
	}
	return 0;
}

/** \brief Create a new RSS feed item
 *
 * \return New feed item.
 */
feed_item newFeedItem(void) {
	feed_item i = (feed_item)am_malloc(sizeof(struct feed_item));
	if(i != NULL) {
		i->name = NULL;
		i->url = NULL;
    i->category = NULL;
	}
	return i;
}

/** \brief Free all allocated memory associated with the given feed item
 *
 * \param[in] data Pointer to a feed item
 */
void freeFeedItem(void *data) {
	feed_item item = (feed_item)data;
	if(item != NULL) {
		if(item->name != NULL) {
			am_free(item->name);
			item->name = NULL;
		}
		if(item->url != NULL) {
			am_free(item->url);
			item->url = NULL;
		}
		if(item->category != NULL) {
			am_free(item->category);
			item->category = NULL;
		}
		am_free(item);
		item = NULL;
	}
}
