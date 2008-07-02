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


int bucket_hasURL(const char *url, NODE *head) {
	NODE *p = head;
	char *x;

	while (p != NULL) {
		x = (char*)p->data;
		if(strcmp(x, url) == 0) {
			return 1;
		}
		p = p->next;
	}
	return 0;
}

int addToBucket(char* identifier, NODE **head, int maxBucketItems) {

	addItem(am_strdup(identifier), head);
	if(maxBucketItems > 0 && listCount(*head) > maxBucketItems) {
		dbg_printf(P_INFO, "[add_to_bucket] bucket gets too large, deleting head item...\n");
		removeLast(*head, NULL);
	}
	return 0;
}
