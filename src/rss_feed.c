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

#include "rss_feed.h"
#include "list.h"
#include "utils.h"
#include "output.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif


void feed_printList(simple_list list) {
#ifdef DEBUG
	NODE *cur = list;
	rss_feed x;

	dbg_printf(P_INFO2, "------- start -------------\n");
	while(cur != NULL && cur->data != NULL) {
		dbg_printf(P_INFO2, "data: (%p)\n", (void*)cur->data);
		x = (rss_feed)cur->data;
		if(x->url != NULL) {
			dbg_printf(P_INFO2, "  url: %s (%p)\n", x->url, (void*)x->url);
		}
		dbg_printf(P_INFO2, "  ttl: %d\n", x->ttl);
		dbg_printf(P_INFO2, "  count: %d\n", x->count);
		dbg_printf(P_INFO2, "  next: (%p)\n", (void*)cur->next);
		cur = cur->next;
	}
	dbg_printf(P_INFO2, "------- end  -------------\n");
#endif
}


rss_feed feed_new(void) {
	rss_feed i = (rss_feed)am_malloc(sizeof(struct rss_feed));
	if(i != NULL) {
		i->url = NULL;
		i->ttl = -1;
		i->count = -1;
	}
	return i;
}

void feed_add(char *url, NODE **head) {
	rss_feed x = feed_new();

	x->url = am_strdup(url);
	addItem(x, head);
}


void feed_free(void* listItem) {
	rss_feed x = (rss_feed)listItem;

	if(x != NULL) {
		if(x->url != NULL) {
			dbg_printf(P_MSG, "freeing feed URL: %s (%p)", x->url, (void*)x->url);
			am_free(x->url);
			x->url = NULL;
		}
		am_free(x);
		x = NULL;
	}
}
