#ifndef RSS_LIST_H__
#define RSS_LIST_H__

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

#include "list.h"

typedef struct rss_item *rss_item;
typedef simple_list rss_list;

struct rss_item {
	char *name;
	char *url;
};

int 		rss_hasURL(const char *url, rss_list head);
void 		rss_printList(rss_list list);
void 		rss_freeItem(void* listItem);
void rss_addItem(char *name, char* url, NODE **head);

rss_item rss_newItem(void);
void 		rss_freeList(NODE **head);
void 		rss_removeLast(NODE *head);
void 		rss_removeFirst(NODE **head);

#endif
