/*
 * transmission.c
 *
 *  Created on: Oct 13, 2008
 *      Author: aurich
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>

#include "output.h"


int8_t call_transmission(const char* tm_path, const char *filename) {
	char buf[PATH_MAX];
	int8_t status;

	if(filename && strlen(filename) > 1) {
		snprintf(buf, PATH_MAX, "transmission-remote -g \"%s\" -a \"%s\"", tm_path, filename);
		dbg_printf(P_INFO, "[call_transmission] calling transmission-remote...");
		dbg_printf(P_INFO2, "[call_transmission] call: %s", buf);
		status = system(buf);
		dbg_printf(P_DBG, "[call_transmission] WEXITSTATUS(status): %d", WEXITSTATUS(status));
		if(status == -1) {
			dbg_printf(P_ERROR, "\n[call_transmission] Error calling sytem(): %s", strerror(errno));
			return -1;
		} else {
			dbg_printf(P_INFO, "[call_transmission] Success");
			return 0;
		}
	} else {
		dbg_printf(P_ERROR, "[call_transmission] Error: invalid filename (addr: %p)", (void*)filename);
		return -1;
	}
}
