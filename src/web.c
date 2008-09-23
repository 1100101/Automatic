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
#include "utils.h"
#include "base64.h"

#define DATA_BUFFER 1024 * 100
#define HEADER_BUFFER 500

static regex_t *content_disp_preg = NULL;

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

static struct HTTPData* HTTPData_new(void) {
	HTTPData* data;

	data = am_malloc(sizeof(struct HTTPData));
	if(!data) {
		return NULL;
	}
	data->data = NULL;
	data->size = 0;
	return data;
}

static void HTTPData_free(HTTPData* data) {
	if(data)
		am_free(data->data);
	am_free(data);
}

struct WebData* WebData_new(const char *url) {
	WebData *data;

	data = am_malloc(sizeof(WebData));
	if(!data)
		return NULL;

	data->url = NULL;
	data->content_filename = NULL;
	data->content_length = -1;
	data->header = NULL;
	data->response = NULL;

	if(url) {
		data->url = am_strdup((char*)url);
	}

	data->header = HTTPData_new();
	if(!data->header) {
		WebData_free(data);
		return NULL;
	}

	data->response = HTTPData_new();
	if(!data->response) {
		WebData_free(data);
		return NULL;
	}
	return data;
}

void WebData_free(struct WebData *data) {
	if(data) {
		am_free(data->url);
		am_free(data->content_filename);
		HTTPData_free(data->header);
		HTTPData_free(data->response);
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
	if(res != 0) {
			dbg_printf(P_ERROR, "[getHTTPData] '%s': %s", url, curl_easy_strerror(res));
			WebData_free(data);
			return NULL;
	} else {
		if(rc != 200) {
			dbg_printf(P_ERROR, "[getHTTPData] Failed to download '%s' [response: %ld]", url, rc);
			WebData_free(data);
			return NULL;
		} else {
			data->header->data = am_realloc(data->header->data, data->header->size + 1);
			data->response->data = am_realloc(data->response->data, data->response->size + 1);
			return data;
		}
	}
}


static int call_transmission(auto_handle *ah, const char *filename) {
	char buf[MAXPATHLEN];
	int8_t status;

	if(filename && strlen(filename) > 1) {
		snprintf(buf, MAXPATHLEN, "transmission-remote -g \"%s\" -a \"%s\"", ah->transmission_path, filename);
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


static char* makeJSON(void *data, uint32_t tsize, uint32_t *setme_size) {

	char *encoded = NULL;
	uint32_t enc_size, json_size;
	char *buf = NULL;
	int buf_size;
	const char *JSONstr =
		"{\n"
		"\"method\": \"torrent-add\",\n"
		"\"arguments\": {\n"
		"\"metainfo\": \"%s\"\n"
		"}\n"
		"}";

	if(data != NULL) {
		encoded = base64_encode(data, tsize, &enc_size);
		if(encoded && enc_size > 0) {
			buf_size = enc_size + 70;
			buf = am_malloc(buf_size);
			json_size = snprintf(buf, buf_size, JSONstr, encoded);
			if(json_size < 0 || json_size >= buf_size) {
				dbg_printf(P_ERROR, "Error producing JSON string with Base64-encoded metadata: %s", strerror(errno));
				am_free(encoded);
				am_free(buf);
				return NULL;
			}
			buf[json_size] = '\0';
			if(setme_size) {
				*setme_size = json_size;
			}
			am_free(encoded);
			return buf;
		}
	}
	return NULL;
}

static int saveFile(const char *name, void *data, uint32_t size) {
	int torrent, ret = -1;

	if(!name  || !data) {
		return -1;
	}

	torrent = open(name, O_RDWR|O_CREAT, 00644);
	if(torrent == -1) {
		dbg_printf(P_ERROR, "Error opening file for writing: %s", strerror(errno));
		ret = -1;
	} else {
		ret = write(torrent, data, size);
		if(ret != size) {
			dbg_printf(P_ERROR, "[saveFile] Error writing torrent file: %s", strerror(errno));
			ret = -1;
		} else {
			dbg_printf(P_INFO, "Saved torrent file '%s'", name);
		}
		fchmod(torrent, 0644);
		close(torrent);
		ret = 0;
	}
	return ret;
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

static char* parseResponse(char* response) {
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
	if(!err) {			/* regex matches */
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


static int uploadData(const char *host, int port, const char* auth, void *data, uint32_t data_size) {
	CURL *curl_handle;
	CURLcode res;
	long rc;
	struct curl_slist *headers = NULL;
	WebData* response_data = NULL;
	char *response_str = NULL;
	char * url = NULL;
	int ret = -1;
	int url_length;

	if(!data || !host) {
		return -1;
	}

	url_length = strlen(host) + 32;
	url = am_malloc(url_length);

	if(url) {
		snprintf( url, url_length, "http://%s:%d/transmission/rpc", host, port );

		response_data = WebData_new(NULL);

		headers = curl_slist_append(headers, "Content-Type: application/json");

		curl_global_init(CURL_GLOBAL_ALL);
		curl_handle = curl_easy_init();
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);

		if(auth) {
			dbg_printf(P_INFO2, "aut: %s", auth);
			curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
			curl_easy_setopt(curl_handle, CURLOPT_USERPWD, auth);
		}

		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data_size);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, response_data);

		curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0);

		res = curl_easy_perform(curl_handle);

		if(res != 0) {
				dbg_printf(P_ERROR, "[uploadData] Failed to upload to '%s': %s", url, curl_easy_strerror(res));
				ret = -1;
		} else {
			curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &rc);
			dbg_printf(P_INFO2, "[uploadData] response code: %ld", rc);
			if(rc != 200) {
				dbg_printf(P_ERROR, "[uploadData] Failed to upload to '%s' [response: %ld]", url, rc);
				ret = -1;
			} else {
				response_str = parseResponse(response_data->response->data);
				if(!strncmp(response_str, "success", 7)) {
					dbg_printf(P_MSG, "Torrent upload successful!");
					ret = 0;
				}else if(!strncmp(response_str, "duplicate torrent", 17)) {
					dbg_printf(P_MSG, "Duplicate Torrent");
					ret = 0;
				} else {
					dbg_printf(P_ERROR, "Error uploading torrent: %s", response_str);
					ret = -1;
				}
			}
		}
		curl_easy_cleanup(curl_handle);
		curl_slist_free_all(headers);
		am_free(response_str);
		am_free(url);
		WebData_free(response_data);
	}
	return ret;
}

uint8_t uploadTorrent(auto_handle *ses, void* t_data, int t_size) {
	char *packet = NULL;
	uint32_t packet_size = 0;
	uint8_t res = 1;

	packet = makeJSON(t_data, t_size, &packet_size);
	if(packet != NULL) {
		res = uploadData(ses->host != NULL ? ses->host : AM_DEFAULT_HOST,ses->rpc_port, ses->auth, packet, packet_size);
		am_free(packet);
	}
	return res;
}

void processTorrent(auto_handle *ah, WebData *wdata, const char *torrent_url) {
	char fname[MAXPATHLEN];
	int res, success = 0;

	if(!ah->use_transmission) {
		get_filename(wdata, fname, ah->torrent_folder);
		res = saveFile(fname, wdata->response->data, wdata->response->size);
		if(res == 0) {
			success = 1;
		}
	} else if(ah->transmission_version == AM_TRANSMISSION_1_2) {
		get_filename(wdata, fname, ah->torrent_folder);
		res = saveFile(fname, wdata->response->data, wdata->response->size);
		if(res == 0) {
			if(call_transmission(ah, fname) == -1) {
				dbg_printf(P_ERROR, "[processTorrent] error adding torrent '%s' to Transmission");
			} else {
				success = 1;
			}
			unlink(fname);
		}
	} else if(ah->transmission_version == AM_TRANSMISSION_1_3) {
		if(uploadTorrent(ah, wdata->response->data, wdata->response->size) == 0) {
			success = 1;
		}
	}

	if(success == 1) {
		if(addToBucket(torrent_url, &ah->downloads, ah->max_bucket_items) == 0) {
			ah->bucket_changed = 1;
		} else {
			dbg_printf(P_ERROR, "Error: Unable to add matched download to bucket list: %s", torrent_url);
		}
	}
}

void getTorrent(auto_handle *ah, const char *url) {
	WebData *wdata;

	wdata = getHTTPData(url);
	if(wdata != NULL && wdata->response != NULL) {
		processTorrent(ah, wdata, url);
	}
	WebData_free(wdata);
}




