#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>
#include "output.h"
#include "list.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

#define MAX_LINE_LEN	300

extern char state_file[MAXPATHLEN + 1];
extern linked_list bucket;

int save_state(NODE *head) {
	FILE *fp;
	NODE *current = head;
	char tmp[MAX_LINE_LEN + 1];

	dbg_printf(P_MSG, "Saving state (%d downloaded torrents) to disk\n", listCount(head));
	if((fp = fopen(state_file, "wb")) == NULL) {
		dbg_printf(P_ERROR, "[save_state] Error: Unable to open '%s' for writing: %s\n", state_file, strerror(errno));
		return -1;
	}
	while (current != NULL) {
		sprintf(tmp, "%s\n", current->item.url);
		if(!fwrite(tmp, strlen(tmp), 1, fp)){
			dbg_printf(P_ERROR, "[save_state] Error: Unable to write to state.txt: %s\n", strerror(errno));
			fclose(fp);
			return -1;
		}
		current = current->pNext;
	}
	fclose(fp);
	return 0;
}

int load_state(NODE **head) {
	FILE *fp;
	char line[MAX_LINE_LEN], str[MAX_LINE_LEN];
	int len;
	rss_item item = newRSSItem();

	if((fp = fopen(state_file, "rb")) == NULL) {
		dbg_printf(P_ERROR, "[save_state] Error: Unable to open state.txt for reading: %s\n", strerror(errno));
		return -1;
	}
	while (fgets(line, MAX_LINE_LEN, fp)) {
		len = strlen(line);
		if(len > 20) {  /* arbitrary limit for an URL */
			strncpy(str, line, len);
			str[len-1] = '\0';
			item.url = str;
			add_to_bucket(item, head, 0);
		}
	}
	fclose(fp);
	dbg_printf(P_MSG, "Read state from file: %d old entries\n", listCount(bucket));
	print_list(bucket);
	return 0;
}

