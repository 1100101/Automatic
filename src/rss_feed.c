/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file rss_feed.c
 *
 * RSS-Feed-specific list functions.
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

#include "rss_feed.h"
#include "list.h"
#include "utils.h"
#include "output.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif


/* private functions */

/** \brief Create a new RSS feed node
 *
 * \return Pointer to the new feed node
 */
PUBLIC rss_feed feed_new(void) {
	rss_feed i = (rss_feed)am_malloc(sizeof(struct rss_feed));
	if(i != NULL) {
		i->url  = NULL;
    i->cookies = NULL;
		i->ttl = -1;
	}
	return i;
}


/* public functions */

/** \brief Print the content of a feed list (URL, TTL, ...)
 *
 * \param list Pointer to a feed list
 */
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
		/*dbg_printf(P_INFO2, "  count: %d\n", x->count);*/
		dbg_printf(P_INFO2, "  next: (%p)\n", (void*)cur->next);
		cur = cur->next;
	}
	dbg_printf(P_INFO2, "------- end  -------------\n");
#endif
}

/** \brief Add a new feed node to a given list
 *
 * \param url URL of the new feed
 * \param head Pointer to a list
 */
PUBLIC void feed_add(rss_feed p, NODE **head) {
  assert(p);
	addItem(p, head);
}


/** \brief Free the memory associated with the given feed-list item
 *
 * \param listItem Pointer to a feed-list item
 *
 * This function is to be used in the generic list functions freeList(), removeFirst() and
 * removeLast() as the 2nd parameter to ensure proper memory deallocation.
 */
void feed_free(void* listItem) {
	rss_feed x = (rss_feed)listItem;

	if(x != NULL) {
		am_free(x->url);
		am_free(x->cookies);
		am_free(x);
	}
}
