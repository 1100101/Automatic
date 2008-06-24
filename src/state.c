/*
 * Copyright (C) 2008 Frank Aurich (1100101+automatic@gmail.com
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>

#include "automatic.h"
#include "output.h"
#include "list.h"


#define MAX_LINE_LEN	300

int save_state(linked_list *bucket) {
	FILE *fp;
	char tmp[MAX_LINE_LEN + 1];
	NODE *current = NULL;
	const char *state_file = am_get_statefile();

	if(state_file) {
		reverseList(bucket);
		current = *bucket;
		dbg_printf(P_MSG, "Saving state (%d downloaded torrents) to disk", listCount(current));
		if((fp = fopen(state_file, "wb")) == NULL) {
			dbg_printf(P_ERROR, "[save_state] Error: Unable to open '%s' for writing: %s", state_file, strerror(errno));
			return -1;
		}
		while (current != NULL && current->item != NULL && current->item->url != NULL) {
			sprintf(tmp, "%s\n", current->item->url);
			if(!fwrite(tmp, strlen(tmp), 1, fp)){
				dbg_printf(P_ERROR, "[save_state] Error: Unable to write to '%s': %s", state_file, strerror(errno));
				fclose(fp);
				return -1;
			}
			current = current->pNext;
		}
		fclose(fp);
	}
	return 0;
}

int load_state(NODE **head) {
	FILE *fp;
	char line[MAX_LINE_LEN];
	int len;
	rss_item item;
	const char* state_file = am_get_statefile();

	if((fp = fopen(state_file, "rb")) == NULL) {
		dbg_printf(P_ERROR, "[load_state] Error: Unable to open '%s' for reading: %s", state_file, strerror(errno));
		return -1;
	}
	while (fgets(line, MAX_LINE_LEN, fp)) {
		len = strlen(line);
		if(len > 20) {  /* arbitrary threshold for the length of an URL */
			item = newRSSItem();
			item->url = malloc(len);
			strncpy(item->url, line, len - 1); /* it's len-1 because of the trailing '\n' from fgets */
			item->url[len - 1] = '\0';
			add_to_bucket(item, head, 0);
			freeRSSItem(item);
		}
	}
	fclose(fp);
	dbg_printf(P_MSG, "Restored %d old entries", listCount(*head));
	return 0;
}

