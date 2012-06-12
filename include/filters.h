#ifndef FILTERS_H__
#define FILTERS_H__

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

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

#include "list.h"
#include "utils.h"

typedef struct am_filter* am_filter;
typedef struct NODE* am_filters;

/** struct representing an RSS feed */
struct am_filter {
	char    *pattern;  /**< Feed URL */
   char    *folder;
   char    *feedID;
};

PUBLIC am_filter filter_new(void);
PUBLIC void filter_free(void* listItem);
PUBLIC void filter_printList(simple_list list);
PUBLIC void filter_add(am_filter p, NODE **head);

#endif  /* FILTERS_H__ */
