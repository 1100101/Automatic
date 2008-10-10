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
#include "utils.h"
#include "output.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif


void pushNode(NODE **ptr_to_head, NODE *newnode) {

	if(*ptr_to_head == NULL) {					/* empty list */
		*ptr_to_head = newnode;
		newnode->next = NULL;
	} else {
		newnode->next = *ptr_to_head;
		*ptr_to_head = newnode;
	}
}


/* public functions */

void reverseList( NODE **list ) {
	NODE *nextnode = NULL;
	NODE *current = NULL;
	NODE *result = NULL;
	current = *list;

	while(current != NULL ) {
		nextnode = current->next;
		current->next = result;
		result = current;
		current = nextnode;
	}
	*list = result;
}

void deleteFirst(NODE **ptr_to_head) {
	NODE *p = NULL;

	if(*ptr_to_head != NULL) {
		p = *ptr_to_head;
		*ptr_to_head = p->next;
		am_free(p);
		p = NULL;
	}
}

unsigned int listCount(NODE *head) {
	int c = 0;
	NODE *current = head;
	while (current != NULL) {
		++c;
		current = current->next;
	}
	return c;
}

/* highly dangerous and only for debugging purposes! */
void printList(NODE *head) {
	NODE *current = head;
	while (current != NULL) {
		if(current->data) {
			dbg_printf(P_DBG, "%s\n", current->data);
		}
		current = current->next;
	}
}

int addItem(void* elem, NODE **head) {
	NODE *newnode = NULL;

	if(elem != NULL) {
		newnode = (NODE*)am_malloc(sizeof(struct NODE));

		if(newnode != NULL) {
			newnode->data = elem;
			newnode->next = NULL;
			pushNode(head, newnode);
			return 0;
		}
		return 1;
	}
	return 1;
}

NODE* getLastNode(NODE *ptr_to_head) {
	NODE *current = ptr_to_head;

	while(current !=NULL && current->next != NULL) {
		current = current->next;
	}
	return current;
}

void deleteLast(NODE **ptr_to_head) {
	NODE *p = *ptr_to_head, *prev = NULL;

	while(p && p->next != NULL) {
		prev = p;
		p = p->next;
	}
	if(prev) {
		prev->next = NULL;
	}
	am_free(p);
	p = NULL;
}

void freeList( NODE **head, listFuncPtr freeFunc ) {
	NODE* node = NULL;

	dbg_printf(P_INFO2, "[cleanupList] size before: %d\n", listCount(*head));
	while (*head != NULL) {
		node = *head;
		*head = (*head)->next;
		if(freeFunc != NULL) {
			freeFunc(node->data);
		} else {
			am_free(node->data);
		}
		am_free(node);
	}
	dbg_printf(P_INFO2, "[cleanupList] size after: %d\n", listCount(*head));
}

void removeLast(NODE *head, listFuncPtr freeFunc) {
	NODE *lastNode = NULL;
	dbg_printf(P_DBG, "Removing last item...\n");
	lastNode = getLastNode(head);

	if(lastNode) {
		if(freeFunc) {
			freeFunc(lastNode->data);
		} else {
			am_free(lastNode->data);
		}
	}
	deleteLast(&head);
}

void removeFirst(NODE  **head, listFuncPtr freeFunc) {
	dbg_printf(P_DBG, "Removing first item...\n");
	if (*head != NULL) {
		if(freeFunc) {
			freeFunc((*head)->data);
		} else {
			am_free((*head)->data);
		}
	}
	deleteFirst(head);
}
