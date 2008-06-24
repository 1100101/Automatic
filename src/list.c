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
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#include "list.h"
#include "automatic.h"
#include "output.h"

void pushNode(NODE **ptr_to_head, NODE *newnode) {

	if(*ptr_to_head == NULL) {					/* empty list */
		*ptr_to_head = newnode;					/* new item becomes head of list */
		newnode->pNext = NULL;
	} else {
		newnode->pNext = *ptr_to_head;
		*ptr_to_head = newnode;
	}
}

void deleteLast(NODE **ptr_to_head) {
	NODE *p = *ptr_to_head, *prev = NULL;
	while(p && p->pNext != NULL) {
		prev = p;
		p = p->pNext;
	}
	if(prev) {
		prev->pNext = NULL;
	}
	freeRSSItem(p->item);
	am_free(p);
	p = NULL;
}

void deleteHead(NODE **ptr_to_head) {
	NODE *p;

	if(*ptr_to_head != NULL) {
		p = *ptr_to_head;
		*ptr_to_head = p->pNext;
		freeRSSItem(p->item);
		am_free(p);
		p = NULL;
	}
}

/* public functions */

void reverseList( NODE **list ) {
	NODE *next;
	NODE *current ;
	NODE *result = NULL;
	current = *list;

	while(current != NULL )	{
		next = current->pNext;
		current->pNext = result;
		result = current;
		current = next;
	}
	*list = result;
}

int addItem(rss_item elem, NODE **head) {
	NODE *pNode;

	if(elem != NULL) {
		pNode = (NODE*)am_malloc(sizeof(struct NODE));

		if(pNode != NULL) {
			pNode->item = elem;
			pNode->pNext = NULL;
			pushNode(head, pNode);
			return 0;
		}
		return 1;
	}
	return 1;
}

int hasURL(const char *url, NODE *head) {
	NODE *p = head;

	while (p != NULL) {
		if(strcmp(p->item->url, url) == 0) {
			return 1;
		}
		p = p->pNext;
	}
	return 0;
}

unsigned int listCount(NODE *head) {
	int c = 0;
	NODE *current = head;
	while (current != NULL) {
		++c;
		current = current->pNext;
	}
	return c;
}

int add_to_bucket(rss_item elem, NODE **b, int max_bucket_items) {
	rss_item newelem;
	int name_set = 0, url_set = 0;

	/* copy element - original will be deleted later */
	if(elem != NULL) {
		newelem = newRSSItem();
		if(elem->name) {
			newelem->name = malloc(strlen(elem->name) + 1);
			strncpy(newelem->name, elem->name, strlen(elem->name) + 1);
			name_set = 1;
		}
		if(elem->url) {
			newelem->url = malloc(strlen(elem->url) + 1);
			strncpy(newelem->url, elem->url, strlen(elem->url) + 1);
			url_set = 1;
		}
		if(name_set || url_set) {			/* don't add empty elements */
			addItem(newelem, b);
		}
		if(max_bucket_items > 0 && listCount(*b) > max_bucket_items) {
			dbg_printf(P_INFO2, "[add_to_bucket] bucket gets too large, deleting head item...\n");
			deleteLast(b);
		}
		return 0;
	}
	return 1;
}

void cleanupList(NODE **list) {
	dbg_printf(P_DBG, "[cleanupList] size before: %d", listCount(*list));
	while (*list != NULL) {
		deleteHead(list);
	}
	dbg_printf(P_DBG, "[cleanupList] size after: %d", listCount(*list));
}

void printList(linked_list list) {
	NODE *cur = list;
	while(cur != NULL && cur->item != NULL) {
		dbg_printf(P_DBG, "item: (%p)", (void*)cur->item);
		if(cur->item->name != NULL) {
			dbg_printf(P_DBG, "  name: %s (%p)", cur->item->name, (void*)cur->item->name);
		}
		if(cur->item->url != NULL) {
			dbg_printf(P_DBG, "  url: %s (%p)", cur->item->url, (void*)cur->item->url);
		}
		dbg_printf(P_DBG, "  next: (%p)", (void*)cur->pNext);
		cur = cur->pNext;
	}
}




rss_item newRSSItem(void) {
	rss_item i = (rss_item)am_malloc(sizeof(struct rss_obj));
	if(i != NULL) {
		i->name = NULL;
		i->url = NULL;
	}
	return i;
}

void freeRSSItem(rss_item listItem) {

	if(listItem != NULL) {
		if(listItem->name != NULL) {
			dbg_printf(P_DBG, "freeing name: %s (%p)", listItem->name, (void*)listItem->name);
			am_free(listItem->name);
			listItem->name = NULL;
		}
		if(listItem->url != NULL) {
			dbg_printf(P_DBG, "freeing url: %s (%p)", listItem->url, (void*)listItem->url);
			am_free(listItem->url);
			listItem->url = NULL;
		}
		am_free(listItem);
		listItem = NULL;
	}
}
