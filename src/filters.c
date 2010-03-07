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
 * Copyright (C) 2010 Frank Aurich (1100101+automatic@gmail.com)
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

#include "filters.h"
#include "list.h"
#include "utils.h"
#include "output.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

/** \brief Create a new RSS feed node
 *
 * \return Pointer to the new feed node
 */
PUBLIC am_filter filter_new(void) {
	am_filter i = (am_filter)am_malloc(sizeof(struct am_filter));
	if(i != NULL) {
		i->pattern = NULL;
		i->folder = NULL;
	}
	return i;
}


/* public functions */

/** \brief Print the content of a feed list (URL, TTL, ...)
 *
 * \param list Pointer to a feed list
 */
PUBLIC void filter_printList(simple_list list) {
#ifdef DEBUG
	NODE *cur = list;
	am_filter x;

	dbg_printf(P_INFO2, "\n------- filter list -------------");
	while(cur != NULL && cur->data != NULL) {
		dbg_printf(P_INFO2, "data: (%p)", (void*)cur->data);
		x = (am_filter)cur->data;
		if(x->pattern != NULL) {
			dbg_printf(P_INFO2, "  pattern: %s (%p)", x->pattern, (void*)x->pattern);
		}
		if(x->folder != NULL) {
			dbg_printf(P_INFO2, "  folder: %s (%p)", x->folder, (void*)x->folder);
		}
		dbg_printf(P_INFO2, "  next: (%p)", (void*)cur->next);
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
PUBLIC void filter_add(am_filter p, NODE **head) {
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
PUBLIC void filter_free(void* listItem) {
	am_filter x = (am_filter)listItem;

	if(x != NULL) {
	  am_free(x->pattern);
		x->pattern = NULL;
	  am_free(x->folder);
		x->folder = NULL;
		am_free(x);
		x = NULL;
	}
}
