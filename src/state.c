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
#include "utils.h"
#include "bucket_list.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif



#define MAX_LINE_LEN	300

int save_state(simple_list *downloads) {
	FILE *fp;
	char tmp[MAX_LINE_LEN + 1];
	NODE *current = NULL;
	const char *state_file = am_get_statefile();

	if(state_file) {
		reverseList(downloads);
		current = *downloads;
		dbg_printf(P_MSG, "Saving state (%d downloaded torrents) to disk", listCount(current));
		if((fp = fopen(state_file, "wb")) == NULL) {
			dbg_printf(P_ERROR, "Error: Unable to open statefile '%s' for writing: %s", state_file, strerror(errno));
			return -1;
		}
		while (current != NULL && current->data != NULL) {
			sprintf(tmp, "%s\n", (char*)current->data);
			if(!fwrite(tmp, strlen(tmp), 1, fp)){
				dbg_printf(P_ERROR, "Error: Unable to write to statefile '%s': %s", state_file, strerror(errno));
				fclose(fp);
				return -1;
			}
			current = current->next ;
		}
		fclose(fp);
	}
	return 0;
}

int load_state(NODE **head) {
	FILE *fp;
	int len;
	char line[MAX_LINE_LEN];
	char *data;
	const char* state_file = am_get_statefile();


	if((fp = fopen(state_file, "rb")) == NULL) {
		dbg_printf(P_ERROR, "[load_state] Error: Unable to open statefile '%s' for reading: %s", state_file, strerror(errno));
		return -1;
	}
	while (fgets(line, MAX_LINE_LEN, fp)) {
		len = strlen(line);
		if(len > 20) {  /* arbitrary threshold for the length of a URL */
			data = am_strndup(line, len-1);  /* len-1 to get rid of the \n at the end of each line */
			addItem(data, head);
		}
	}
	fclose(fp);
	dbg_printf(P_MSG, "Restored %d old entries", listCount(*head));
	return 0;
}

