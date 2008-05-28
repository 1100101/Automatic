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
#include "list.h"
#include "output.h"

extern int max_bucket_items;

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

static void freeItem(NODE *item);

void insertNode(NODE **ptr_to_head, NODE *newnode) {
	NODE /* *p, */ *prev;

	/* link new element into existing list */
	prev = NULL;
	if (prev == NULL) {						/* insertion at head of list */
		newnode->pNext = *ptr_to_head; 		/* append old list (if any) to new node */
		*ptr_to_head = newnode;				/* new item becomes head of list */
	} else {								/* insert between prev and p */
		prev->pNext = newnode;
		newnode->pNext = NULL;
	}
}

void deleteNode(NODE **ptr_to_head, NODE *pNode) {
	NODE *p, *prev = NULL;

	for (p = *ptr_to_head; p != NULL; prev = p, p = p->pNext) {
		if (p == pNode) {
			if (prev == NULL) {				/* deletion at head of list */
				*ptr_to_head = p->pNext;
			} else {
				prev->pNext = p->pNext;		/* make predecessor point to successor */
			}
			free(p);								/* release memory */
			return;
		}
	}
}

/* public functions */

rss_item newRSSItem() {
	rss_item i;
	i.name = NULL;
	i.url = NULL;
	return i;
}

void freeList(NODE **ptr_to_head) {
	NODE *p = *ptr_to_head;

	while (*ptr_to_head != NULL) {
		p = (*ptr_to_head)->pNext;
		/* release element and check pointer after "free" */
		free(*ptr_to_head);
		/* deleteNode(ptr_to_head, *ptr_to_head); */
		*ptr_to_head = p;
	}
	return;
}

void addRSSItem(rss_item elem, NODE **head) {
	NODE *pNode = (NODE*)malloc(sizeof(NODE));
	if(pNode == NULL)
		perror("malloc failed");
	dbg_printf(P_INFO2, "[addRSSItem] allocated %d bytes for pNode\n", sizeof(NODE));
	pNode->item = elem;
	pNode->pNext = NULL;
	insertNode(head, pNode);
}

int hasURL(const char *url, NODE *head) {
	NODE *p = head;

	while (p != NULL) {
		if(strcmp(p->item.url, url) == 0) {
			return 1;
		}
		p = p->pNext;
	}
	return 0;
}

void deleteHead(NODE **ptr_to_head) {
	freeItem(*ptr_to_head);
 	deleteNode(ptr_to_head, *ptr_to_head);
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

int add_to_bucket(rss_item elem, NODE **b, int use_size_limit) {
	rss_item b_elem = newRSSItem();
	/* copy element content, as the original is going to be freed later on */
	int integrity_check = 1;

	if(elem.name != NULL) {
		dbg_printf(P_INFO2, "[add_to_bucket] elem.name: %s\n", elem.name);
		b_elem.name = malloc(strlen(elem.name) + 1);
		if(b_elem.name) {
			dbg_printf(P_INFO2, "[add_to_bucket] allocated %d bytes for '%s'\n", strlen(elem.name) + 1, elem.name);
			strcpy(b_elem.name, elem.name);
		} else {
			dbg_printf(P_ERROR, "[add_to_bucket] malloc failed: %s\n", strerror(errno));
			integrity_check &= 0;
		}
	}
	if(elem.url != NULL) {
		dbg_printf(P_INFO2, "[add_to_bucket] elem.url: %s\n", elem.url);
		b_elem.url = malloc(strlen(elem.url) + 1);
		if(b_elem.url) {
			dbg_printf(P_INFO2, "[add_to_bucket] allocated %d bytes for '%s'\n", strlen(elem.url) + 1, elem.url);
			strcpy(b_elem.url, elem.url);
		} else {
			dbg_printf(P_ERROR, "[add_to_bucket] malloc failed: %s\n", strerror(errno));
			integrity_check &= 0;
		}
	}
	if(integrity_check) {
		addRSSItem(b_elem, b);
		if(use_size_limit && listCount(*b) > max_bucket_items) {
			dbg_printf(P_INFO, "[add_to_bucket] bucket gets too large, deleting head item...\n");
			deleteHead(b);
		}
	}
	return integrity_check;
}


static void freeItem(NODE *item) {
	if(item != NULL) {
		if(item->item.name) {
			free(item->item.name);
			dbg_printf(P_INFO2, "[freeItem] freed 'elem.name'\n");
		}
		if(item->item.url) {
			free(item->item.url);
			dbg_printf(P_INFO2, "[freeItem] freed 'elem.url'\n");
		}
	}
}

void cleanup_list(NODE **list) {
	NODE *current = *list;
	dbg_printf(P_INFO2, "[cleanup_list] list size before: %d\n", listCount(*list));
	while (current != NULL) {
		freeItem(current);
		current = current->pNext;
	}
	freeList(list);
	dbg_printf(P_INFO2, "[cleanup_list] list size after: %d\n", listCount(*list));
}

void print_list(linked_list list) {
	NODE *cur = list;
	while(cur != NULL) {
		if(cur->item.name != NULL) {
			dbg_printf(P_INFO2, "  name:\t%s\n", cur->item.name);
		}
		if(cur->item.url != NULL) {
			dbg_printf(P_INFO2, "  URL:\t%s\n", cur->item.url);
		}
		cur = cur->pNext;
	}
}
