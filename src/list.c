/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file list.c
 *
 * Functions for generic single-linked lists
 */

/* Copyright (C) 2008 Frank Aurich (1100101+automatic@gmail.com)
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


/* private functions */
static void addNodeFirst(NODE **ptr_to_head, NODE *newnode) {

	if(*ptr_to_head == NULL) {					/* empty list */
		*ptr_to_head = newnode;
		newnode->next = NULL;
	} else {
		newnode->next = *ptr_to_head;
		*ptr_to_head = newnode;
	}
}

static void addNodeLast(NODE **ptr_to_head, NODE *newnode) {
	NODE *current = *ptr_to_head;

	if(current == NULL) {					/* empty list */
		*ptr_to_head = newnode;
		newnode->next = NULL;
	} else {
		while(current != NULL && current->next != NULL) {
			current = current->next;
		}
		current->next = newnode;
		newnode->next = NULL;
	}
}

static void deleteFirst(NODE **ptr_to_head) {
	NODE *p = NULL;

	if(*ptr_to_head != NULL) {
		p = *ptr_to_head;
		*ptr_to_head = p->next;
		am_free(p);
		p = NULL;
	}
}

static void deleteLast(NODE **ptr_to_head) {
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

static NODE* getLastNode(NODE *ptr_to_head) {
	NODE *current = ptr_to_head;

	while(current != NULL && current->next != NULL) {
		current = current->next;
	}
	return current;
}

/* public functions */

/** \brief Reverse the order of a list
 *
 * \param[in,out] list The List
 *
 * Exchanges the node items within a list so the first element becomes the last
 * and vice versa.
 */

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

/** \brief Count the elements in a list
 *
 * \param[in] head Pointer to a list
 * \return Number of nodes in the list.
 */
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

/** \brief Print the content of a list.
 *
 * \param[in] head Pointer to a list
 *
 * \warning This function expects that the data element of each node points to a string!
 * If that is not the case it will most likely crash. Use it \b ONLY for debugging and only
 * if you're certain that it's a list of strings!
 */
void printList(const simple_list head) {
	NODE *current = head;
	while (current != NULL) {
		if(current->data) {
			dbg_printf(P_MSG, "\t%s", (char*)current->data);
		}
		current = current->next;
	}
}

/** \brief Add a new item to a list.
 *
 * \param[in] elem Pointer to data.
 * \param[in,out] head Pointer to a list
 * \return 0 if element was successfully added to the list, 1 otherwise.
 *
 * The function creates a new list node which points to data. The function does not care
 * about the type of data.
 */

int addItem(void* elem, NODE **head) {
	return addToHead(elem, head);
}

int addToHead(void* elem, NODE **head) {
	NODE *newnode = NULL;

	if(head != NULL && elem != NULL) {
		newnode = (NODE*)am_malloc(sizeof(struct NODE));

		if(newnode != NULL) {
			newnode->data = elem;
			newnode->next = NULL;
			addNodeFirst(head, newnode);
			return 0;
		}
		return -1;
	}
	return -1;
}

int addToTail(void* elem, NODE **head) {
	NODE *newnode = NULL;

	if(head != NULL && elem != NULL) {
		newnode = (NODE*)am_malloc(sizeof(struct NODE));

		if(newnode != NULL) {
			newnode->data = elem;
			newnode->next = NULL;
			addNodeLast(head, newnode);
			return 0;
		}
		return -1;
	}
	return -1;
}

/** \brief Free a list and all memory associated with it
 *
 * \param[in,out] head Pointer to a list
 * \param[in] freeFunc function pointer for freeing the memory of each list item
 *
 * The function removes all items from a list, freeing all associated memory.
 * If freeFunc is NULL, it tries to directly free the data of each node.
 * For complex nodes (e.g. structs) the user needs to provide a node-specific free() function.
 */
void freeList( NODE **head, listFuncPtr freeFunc ) {
	NODE* node = NULL;

	dbg_printf(P_INFO2, "[cleanupList] size before: %d", listCount(*head));
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
	dbg_printf(P_INFO2, "[cleanupList] size after: %d", listCount(*head));
}

/** \brief Remove the last item of a list
 *
 * \param[in,out] head Pointer to a list
 * \param[in] freeFunc Function pointer for freeing the memory of the list item
 *
 * Removes the last element of the given list and frees all associated memory.
 * If freeFunc is NULL, it tries to directly free the \a data item of each node.
 * This is useful if \a data is a pointer to a single element (e.g. string, int, ...)
 * For complex elements (e.g. structs) the user needs to provide a node-specific free() function.
 * (For example freeFeedItem() )
 */
void removeLast(NODE *head, listFuncPtr freeFunc) {
	NODE *lastNode = NULL;
	dbg_printf(P_DBG, "Removing last item...");
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

/** \brief Remove the first item of a list
 *
 * \param[in,out] head Pointer to a list
 * \param[in] freeFunc Function pointer for freeing the memory of the list item
 *
 * Removes the first element of the given list and frees all associated memory.
 * If freeFunc is NULL, it tries to directly free the data of each node.
 * For complex nodes (e.g. structs) the user needs to provide a node-specific free() function.
 */
void removeFirst(NODE  **head, listFuncPtr freeFunc) {
	dbg_printf(P_DBG, "Removing first item...");
	if (*head != NULL) {
		if(freeFunc) {
			freeFunc((*head)->data);
		} else {
			am_free((*head)->data);
		}
		deleteFirst(head);
	}
}
