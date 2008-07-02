#ifndef LIST_H__
#define LIST_H__

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

#ifdef MEMWATCH
	#include "memwatch.h"
#endif


typedef struct NODE NODE;
typedef struct NODE* simple_list;

struct NODE {
   NODE *next;
   void *data;
};

typedef void (*listFuncPtr)(void *);

int addItem(void *elem, NODE **head);

void freeList( NODE **head, listFuncPtr freeFunc );
void removeFirst(NODE  **head, listFuncPtr freeFunc);
void removeLast(NODE *head, listFuncPtr freeFunc);


unsigned int listCount(NODE* head);
void reverseList(NODE **list);
NODE* getLastNode(NODE *ptr_to_head);

#endif
