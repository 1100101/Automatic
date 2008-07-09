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

#include "list.h"
#include "feed_item.h"
#include "utils.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

feed_item newFeedItem(void) {
	feed_item i = (feed_item)am_malloc(sizeof(struct feed_item));
	if(i != NULL) {
		i->name = NULL;
		i->url = NULL;
	}
	return i;
}

void freeFeedItem(feed_item item) {
	if(item != NULL) {
		if(item->name != NULL) {
			am_free(item->name);
			item->name = NULL;
		}
		if(item->url != NULL) {
			am_free(item->url);
			item->url = NULL;
		}
		am_free(item);
		item = NULL;
	}
}
