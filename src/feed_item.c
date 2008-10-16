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
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <regex.h>
#include <stdint.h>


#include "feed_item.h"
#include "output.h"
#include "utils.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

uint8_t isMatch(const simple_list filters, const feed_item item) {
	int err;
	regex_t preg;
	char erbuf[100];
	char *regex_str;
	simple_list current_regex;

	current_regex = filters;
	while (current_regex != NULL && current_regex->data != NULL) {
		regex_str = (char*) current_regex->data;
		dbg_printf(P_INFO2, "Current regex: %s", regex_str);
		err = regcomp(&preg, regex_str, REG_EXTENDED | REG_ICASE);
		if (err) {
			regerror(err, &preg, erbuf, sizeof(erbuf));
			dbg_printf(P_INFO2, "regcomp: Error compiling regular expression: %s",
					erbuf);
			current_regex = current_regex->next;
			continue;
		}
		dbg_printf(P_INFO2, "Current feed_item: %s", item->name);
		err = regexec(&preg, item->name, 0, NULL, 0);
		// if(!err && !has_been_downloaded(session->downloads, item->url)) {  regex matches and it hasn't been downloaded before
		if (!err) {
			regfree(&preg);
			return 1;
		} else if (err != REG_NOMATCH && err != 0) {
			regerror(err, &preg, erbuf, sizeof(erbuf));
			dbg_printf(P_ERROR, "[check_for_downloads] regexec error: %s", erbuf);
		}
		regfree(&preg);
		current_regex = current_regex->next;
	}
	return 0;
}


feed_item newFeedItem(void) {
	feed_item i = (feed_item)am_malloc(sizeof(struct feed_item));
	if(i != NULL) {
		i->name = NULL;
		i->url = NULL;
	}
	return i;
}

void freeFeedItem(void *data) {
	feed_item item = (feed_item)data;
	if(item != NULL) {
		if(item->name != NULL) {
			am_free(item->name);
			item->name = NULL;
		}
		if(item->url != NULL) {
			am_free(item->url);
			item->url = NULL;
		}
		am_free(item);
		item = NULL;
	}
}
