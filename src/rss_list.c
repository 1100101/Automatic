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
#include "rss_list.h"
#include "utils.h"
#include "output.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif


int rss_hasURL(const char *url, NODE *head) {
	NODE *p = head;
	rss_item x;

	while (p != NULL) {
		x = (rss_item)p->data;
		if(strcmp(x->url, url) == 0) {
			return 1;
		}
		p = p->next;
	}
	return 0;
}

void rss_freeList(NODE **head) {
	/*rss_item x;
	dbg_printf(P_INFO2, "[cleanupList] size before: %d", listCount(*head));
	while (*head != NULL) {
		if((*head)->item) {
			x = (rss_item)(*head)->item;
			rss_freeItem(x);
		}
		deleteFirst(head);
	}
	dbg_printf(P_INFO2, "[cleanupList] size after: %d", listCount(*head));*/
	freeList(head, rss_freeItem);
}

void rss_printList(linked_list list) {
#ifdef DEBUG
	NODE *cur = list;
	rss_item x;
	dbg_printf(P_INFO2, "------- start -------------\n");
	while(cur != NULL && cur->data != NULL) {
		dbg_printf(P_INFO2, "data: (%p)\n", (void*)cur->data);
		x = (rss_item)cur->data;
		if(x->name != NULL) {
			dbg_printf(P_INFO2, "  name: %s (%p)\n", x->name, (void*)x->name);
		}
		if(x->url != NULL) {
			dbg_printf(P_INFO2, "  url: %s (%p)\n", x->url, (void*)x->url);
		}
		dbg_printf(P_INFO2, "  next: (%p)\n", (void*)cur->next);
		cur = cur->next;
	}
	dbg_printf(P_INFO2, "------- end  -------------\n");
#endif
}

void rss_removeLast(NODE *head) {
	removeLast(head, rss_freeItem);
}

void rss_removeFirst(NODE  **head) {
	removeFirst(head, rss_freeItem);
}

void rss_addItem(char *title, char* url, NODE **head) {
	rss_item x = rss_newItem();

	x->name = am_strdup(title);
	x->url = am_strdup(url);
	dbg_printf(P_DBG, "[addItem] name: %s,\turl: %s", x->name, x->url);
	addItem(x, head);
}


rss_item rss_newItem(void) {
	rss_item i = (rss_item)am_malloc(sizeof(struct rss_item));
	if(i != NULL) {
		i->name = NULL;
		i->url = NULL;
	}
	return i;
}

void rss_freeItem(void* listItem) {
	rss_item x = (rss_item)listItem;

	if(x != NULL) {
		if(x->name != NULL) {
			am_free(x->name);
			x->name = NULL;
		}
		if(x->url != NULL) {
			am_free(x->url);
			x->url = NULL;
		}
		am_free(x);
		x = NULL;
	}
}
