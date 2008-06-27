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


#include <sys/time.h>
#include <sys/types.h>
#include <regex.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <regex.h>
#include <curl/curl.h>
#include <stdint.h>

#include "automatic.h"
#include "web.h"
#include "output.h"

#define DATA_BUFFER 1024 * 100
#define HEADER_BUFFER 500

static regex_t *content_disp_preg = NULL;
static void get_filename(WebData *data, char *file_name, const char *path);

int is_torrent(const char *str) {
	if(strstr(str, ".torrent"))
		return 1;
	else
		return 0;
}

static void init_cd_preg() {
	int err;
	char erbuf[100];
 	const char* fname_regex = "Content-Disposition: (inline|attachment); filename=\"(.+)\"$";

	assert(content_disp_preg == NULL);

	content_disp_preg = am_malloc(sizeof(regex_t));
	dbg_printf(P_INFO2, "[init_cd_preg] allocated %d bytes for content_disp_preg", sizeof(regex_t));
	err = regcomp(content_disp_preg, fname_regex, REG_EXTENDED|REG_ICASE);
	/* a failure of regcomp won't be critical. the app will fall back and extract a filename from the URL */
	if(err) {
		regerror(err, content_disp_preg, erbuf, sizeof(erbuf));
		dbg_printf(P_ERROR, "[init_cd_preg] regcomp: Error compiling regular expression: %s", erbuf);
	}
}

void cd_preg_free() {
	if(content_disp_preg) {
		regfree(content_disp_preg);
		am_free(content_disp_preg);
	}
}

static size_t write_header_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	WebData *mem = data;
	char *buf;
	char tmp[20];
	char *p;
	char erbuf[100];
	int content_length = 0, len, err;
	int i;
	regmatch_t pmatch[3];

	buf = am_malloc(realsize + 1);
	if(!buf) {
		dbg_printf(P_ERROR, "[write_header_callback] Error allocating %d bytes for 'buf'", realsize + 1);
		return realsize;
	}

	memcpy(buf, ptr, realsize);
	buf[realsize] = '\0';

	/* total hack to get rid of the newline/carriage return at the end of each header line */
	for(i = realsize-2; i < realsize; i++) {
		if(buf[i] == '\r' || buf[i] == '\n')
			buf[i] = '\0';
	}

	/* parse header for Content-Length to allocate correct size for data->response->data */
	if(!strncmp(buf, "Content-Length:", 15)) {
		p = strtok(buf, " ");
		while(p != NULL) {
			strcpy(tmp, p);
			p = strtok(NULL, " ");
		}
		content_length = atoi(tmp);
		if(content_length > 0) {
			mem->content_length = content_length;
			mem->response->data = am_realloc(mem->response->data, content_length + 1);
		}
	} else {
		/* parse header for Content-Disposition to get correct filename */
		if(!content_disp_preg) {
			init_cd_preg();
		}
		err = regexec(content_disp_preg, buf, 3, pmatch, 0);
		if(!err) {			/* regex matches */
			len = pmatch[2].rm_eo - pmatch[2].rm_so;
			mem->content_filename = am_realloc(mem->content_filename, len + 1);
			strncpy(mem->content_filename, buf + pmatch[2].rm_so, len);
			mem->content_filename[len] = '\0';
			dbg_printf(P_INFO2, "[write_header_callback] Found filename: %s", mem->content_filename);
		} else if(err != REG_NOMATCH && err != 0){
			len = regerror(err, content_disp_preg, erbuf, sizeof(erbuf));
			dbg_printf(P_ERROR, "[write_header_callback] regexec error: %s", erbuf);
		} else {
			if(!strncmp(buf, "Content-Disposition", 19)) {
				len = regerror(err, content_disp_preg, erbuf, sizeof(erbuf));
				dbg_printf(P_ERROR, "[write_header_callback] regexec error: %s", erbuf);
				dbg_printf(P_ERROR, "[write_header_callback] Unknown Content-Disposition pattern: %s", buf);
			}
		}
	}

	if(!mem->header->data) {
		mem->header->data = am_malloc(HEADER_BUFFER);
		dbg_printf(P_INFO2, "[write_header_callback] allocated %d bytes for mem->header->data", HEADER_BUFFER);
	}

	/* save header line to mem->header */
	if(mem->header->size + realsize + 1 > HEADER_BUFFER) {
		mem->header->data = (char *)am_realloc(mem->header->data, mem->header->size + realsize + 1);
	}
	if (mem->header->data) {
		memcpy(&(mem->header->data[mem->header->size]), ptr, realsize);
		mem->header->size += realsize;
		mem->header->data[mem->header->size] = 0;
	}

	am_free(buf);
	return realsize;
}

static size_t write_data_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	WebData *mem = data;

	/* fallback in case content-length detection in write_header_callback was not successful */
	if(!mem->response->data) {
		mem->response->data = (char*)am_malloc(DATA_BUFFER);
		dbg_printf(P_INFO2, "[write_data_callback] allocated %d bytes for mem->response->data", DATA_BUFFER);
	}
	if(mem->response->size + realsize + 1 > DATA_BUFFER) {
		mem->response->data = (char *)am_realloc(mem->response->data, mem->response->size + realsize + 1);
	}
	if (mem->response->data) {
		memcpy(&(mem->response->data[mem->response->size]), ptr, realsize);
		mem->response->size += realsize;
		mem->response->data[mem->response->size] = 0;
	}
	return realsize;
}

struct WebData* WebData_new(const char *url) {
	WebData *data;
	int len;

	data = am_malloc(sizeof(WebData));
	if(!data)
		return NULL;
	if(url) {
		len = strlen(url);
		data->url = am_malloc(len + 1);
		strncpy(data->url, url, len);
		data->url[len] = '\0';
	}
	data->content_filename = NULL;
	data->content_length = -1;
	data->header = am_malloc(sizeof(HTTPData));
	if(!data->header) {
		am_free(data);
		return NULL;
	}
	data->header->data = NULL;
	data->header->size = 0;
	data->response = am_malloc(sizeof(HTTPData));
	if(!data->response) {
		am_free(data->header);
		am_free(data);
		return NULL;
	}
	data->response->data = NULL;
	data->response->size = 0;
	return data;
}

void WebData_free(struct WebData *data) {
	if(data) {
		am_free(data->url);
		am_free(data->content_filename);
		if(data->response) {
			am_free(data->response->data);
			am_free(data->response);
		}
		if(data->header){
			am_free(data->header->data);
			am_free(data->header);
		}
		am_free(data);
		data = NULL;
	}
}

WebData* getHTTPData(const char *url) {
	CURL *curl_handle;
	CURLcode res;
	char errorBuffer[CURL_ERROR_SIZE];
	long rc;

	WebData* data = WebData_new(url);
	if(!data)
		return NULL;

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errorBuffer);
 	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data_callback);
	curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, write_header_callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, data);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, data);
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	res = curl_easy_perform(curl_handle);
	curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &rc);
	dbg_printf(P_INFO2, "[getHTTPData] response code: %ld", rc);
	curl_easy_cleanup(curl_handle);
	if(rc != 200) {
		dbg_printf(P_ERROR, "[getHTTPData] Failed to get '%s' [response: %ld]", url, rc);
		WebData_free(data);
		return NULL;
	} else {
		data->header->data = am_realloc(data->header->data, data->header->size + 1);
		data->response->data = am_realloc(data->response->data, data->response->size + 1);
		return data;
	}
}

static int call_transmission(const char* transmission_path, const char *filename) {
	char buf[MAXPATHLEN];
	int8_t status;

	if(filename && strlen(filename) > 1) {
		sprintf(buf, "transmission-remote -g \"%s\" -a \"%s\"", transmission_path, filename);
		dbg_printf(P_INFO, "[call_transmission] calling transmission-remote...");
		status = system(buf);
		dbg_printf(P_INFO, "[call_transmission] WEXITSTATUS(status): %d", WEXITSTATUS(status));
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


void download_torrent(auto_handle *ah,rss_item item) {
	char fname[MAXPATHLEN];
	int torrent;
	int res;
	WebData *wdata;

	dbg_printf(P_MSG, "Found new download: %s (%s)", item->name, item->url);
	wdata = getHTTPData(item->url);
	if(wdata && wdata->response) {
		get_filename(wdata, fname, ah->torrent_folder);
		torrent = open(fname,O_RDWR|O_CREAT, 00644);
		if(torrent == -1) {
			dbg_printf(P_ERROR, "Error opening file for writing: %s", strerror(errno));
		} else {
			res = write(torrent, wdata->response->data, wdata->response->size);
			if(res != wdata->response->size) {
				dbg_printf(P_ERROR, "[download_torrent] Error writing torrent file: %s", strerror(errno));
			} else {
				dbg_printf(P_INFO, "Saved torrent file '%s'", fname);
			}
			fchmod(torrent, 0644);
			close(torrent);
			if(ah->use_transmission && call_transmission(ah->transmission_path, fname) == -1) {
				dbg_printf(P_ERROR, "[download_torrent] error adding torrent '%s' to Transmission");
				unlink(fname);
			} else {
				if(add_to_bucket(item, &ah->bucket, ah->max_bucket_items) == 0) {
					ah->bucket_changed = 1;
				} else {
					dbg_printf(P_ERROR, "Error: Unable to add matched download to bucket list: %s", item->name);
				}
			}
		}
	} else {
		dbg_printf(P_ERROR, "Error downloading torrent file (wdata: %p, wdata->response: %p)", (void*)wdata, (void*)(wdata->response));
	}
	WebData_free(wdata);
}


static void get_filename(WebData *data, char *file_name, const char *path) {
	char *p, tmp[MAXPATHLEN], fname[MAXPATHLEN], buf[MAXPATHLEN];
	int len;

#ifdef DEBUG
	assert(data);
	assert(path);
#endif

	if(data->content_filename) {
		strncpy(buf, data->content_filename, strlen(data->content_filename) + 1);
	} else {
		strcpy(tmp, data->url);
		p = strtok(tmp, "/");
		while (p) {
			len = strlen(p);
			if(len < MAXPATHLEN)
				strcpy(buf, p);
			p = strtok(NULL, "/");
		}
	}
	strcpy(fname, path);
	strcat(fname, "/");
	strcat(fname, buf);
	if(!is_torrent(buf)) {
		strcat(fname, ".torrent");
	}
	strcpy(file_name, fname);
}
