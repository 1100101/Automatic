/*
 * torrent.c
 *
 *  Created on: Oct 13, 2008
 *      Author: aurich
 */

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>
#include "output.h"
#include "list.h"
#include "utils.h"
#include "downloads.h"
#include "web.h"


static int is_torrent(const char *str) {
	if(strstr(str, ".torrent"))
	return 1;
	else
	return 0;
}

void get_filename(char *filename, const char *content_filename, const char* url, const char *tm_path) {
	char *p, tmp[PATH_MAX], buf[PATH_MAX];
	int len;


#ifdef DEBUG
	assert(url);
	assert(tm_path);
#endif

	if (content_filename) {
		strncpy(buf, content_filename, strlen(content_filename) + 1);
	} else {
		strcpy(tmp, url);
		p = strtok(tmp, "/");
		while (p) {
			len = strlen(p);
			if (len < PATH_MAX)
				strcpy(buf, p);
			p = strtok(NULL, "/");
		}
	}
	snprintf(filename, PATH_MAX - 1, "%s/%s%s", tm_path, buf, (is_torrent(buf) ? "" : ".torrent"));
}

static char* parseResponse(const char* response) {
	int err, len;
	char erbuf[100];
	regmatch_t pmatch[2];
	const char* result_regex = "\"result\": \"(.+)\"\n";
	regex_t *result_preg = am_malloc(sizeof(regex_t));
	char *result_str = NULL;

	err = regcomp(result_preg, result_regex, REG_EXTENDED|REG_ICASE);
	if(err) {
		regerror(err, result_preg, erbuf, sizeof(erbuf));
		dbg_printf(P_ERROR, "[parseResponse] regcomp: Error compiling regular expression: %s", erbuf);
	}
	err = regexec(result_preg, response, 2, pmatch, 0);
	if(!err) { /* regex matches */
		len = pmatch[1].rm_eo - pmatch[1].rm_so;
		dbg_printf(P_DBG, "result len: %d (end: %d, start: %d)", len, pmatch[1].rm_eo, pmatch[1].rm_so);
		result_str = am_strndup(response + pmatch[1].rm_so, len);
	} else {
		len = regerror(err, result_preg, erbuf, sizeof(erbuf));
		dbg_printf(P_ERROR, "[parseResponse] regexec error: %s", erbuf);
	}
	regfree(result_preg);
	am_free(result_preg);
	return result_str;
}

/*int8_t uploadTorrent(auto_handle *ses, const void *t_data, int t_size) {
	char *packet = NULL, *res = NULL, *response = NULL;
	uint32_t packet_size = 0, ret = -1;
	char url[MAX_URL_LEN];

	packet = makeJSON(t_data, t_size, &packet_size);
	if(packet != NULL) {
		snprintf( url, MAX_URL_LEN, "http://%s:%d/transmission/rpc", (ses->host != NULL) ? ses->host : AM_DEFAULT_HOST, ses->rpc_port);
		res = sendHTTPData(url, ses->auth, packet, packet_size);
		if(res != NULL) {
			response = parseResponse(res);
			if(!strncmp(response, "success", 7)) {
				dbg_printf(P_MSG, "Torrent upload successful!");
				ret = 0;
			} else if(!strncmp(response, "duplicate torrent", 17)) {
				dbg_printf(P_MSG, "Duplicate Torrent");
				ret = 0;
			} else {
				dbg_printf(P_ERROR, "Error uploading torrent: %s", response);
				ret = -1;
			}
			am_free(response);
			am_free(res);
		}
		am_free(packet);
	}
	return ret;
}*/
