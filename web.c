#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>

#include "web.h"
#include "output.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

#define DATA_BUFFER 1024 * 100
#define HEADER_BUFFER 500

static size_t write_header_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	WebData *mem = data;
	char *buf;
	char tmp[20];
	char *p;
	int content_length = 0;
	if(!mem->header->data) {
		mem->header->data = (char*)malloc(HEADER_BUFFER);
		dbg_printf(P_INFO2, "[write_callback] allocated %d bytes for mem->header->data\n", HEADER_BUFFER);
	}
	buf = malloc(realsize + 1);
	memcpy(buf, ptr, realsize);
	buf[realsize] = '\0';
	/* parse header for Content-Length to allocate correct size for data->response->data */
	if(!strncmp(buf, "Content-Length:", 15)) {
		p = strtok(buf, " ");
		while(p != NULL) {
			strcpy(tmp, p);
			p = strtok(NULL, " ");
		}
		content_length = atoi(tmp);
		if(content_length > 0) {
			mem->response->data = realloc(mem->response->data, content_length + 1);
			dbg_printf(P_INFO2, "[write_header_callback] reallocated data->response->data to %d byte (using Content-Length)\n", content_length + 1);
		}
	}
	if(mem->header->size + realsize + 1 > HEADER_BUFFER) {
		mem->header->data = (char *)realloc(mem->header->data, mem->header->size + realsize + 1);
		dbg_printf(P_INFO2, "[write_callback] reallocated %d bytes for mem->data\n", mem->header->size + realsize + 1);
	}
	if (mem->header->data) {
		memcpy(&(mem->header->data[mem->header->size]), ptr, realsize);
		mem->header->size += realsize;
		mem->header->data[mem->header->size] = 0;
	}
	if(buf)
		free(buf);
	return realsize;
}

static size_t write_data_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	WebData *mem = data;
	/* fallback in case content-length detection in write_header_callback was not successful */
	if(!mem->response->data) {
		mem->response->data = (char*)malloc(DATA_BUFFER);
		dbg_printf(P_INFO2, "[write_data_callback] allocated %d bytes for mem->response->data\n", DATA_BUFFER);
	}
	if(mem->response->size + realsize + 1 > DATA_BUFFER) {
		mem->response->data = (char *)realloc(mem->response->data, mem->response->size + realsize + 1);
		dbg_printf(P_INFO2, "[write_data_callback] reallocated %d bytes for mem->data\n", mem->response->size + realsize + 1);
	}
	if (mem->response->data) {
		memcpy(&(mem->response->data[mem->response->size]), ptr, realsize);
		mem->response->size += realsize;
		mem->response->data[mem->response->size] = 0;
	}
	return realsize;
}
/*
static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	HTTPData *mem = data;
	mem->data = (char *)realloc(mem->data, mem->size + realsize + 1);
	if (mem->data) {
		dbg_printf(P_INFO2, "[write_callback] reallocated %d bytes for mem->data\n", mem->size + realsize + 1);
		memcpy(&(mem->data[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->data[mem->size] = 0;
	}
	return realsize;
}
*/

struct WebData* WebData_new(const char *url) {
	WebData *data;
	int len;

	data = malloc(sizeof(WebData));
	if(!data)
		return NULL;
	dbg_printf(P_INFO2, "[WebData_new] allocated %d bytes for wData\n", sizeof(WebData));
	if(url) {
		len = strlen(url);
		data->url = malloc(len + 1);
		strncpy(data->url, url, len);
		data->url[len] = '\0';
	}
	data->header = malloc(sizeof(HTTPData));
	if(!data->header) {
		free(data);
		return NULL;
	}
	dbg_printf(P_INFO2, "[WebData_new] allocated %d bytes for data->header\n", sizeof(HTTPData));
	data->header->data = NULL;
	data->header->size = 0;
	data->response = malloc(sizeof(HTTPData));
	if(!data->response) {
		free(data->header);
		free(data);
		return NULL;
	}
	dbg_printf(P_INFO2, "[WebData_new] allocated %d bytes for data->response\n", sizeof(HTTPData));
	data->response->data = NULL;
	data->response->size = 0;
	return data;
}

void WebData_free(struct WebData *data) {
	if(data) {
		if(data->url) {
			free(data->url);
			dbg_printf(P_INFO2, "[WebData_free] freed data->url\n");
		}
		if(data->response) {
			if(data->response->data) {
				free(data->response->data);
				dbg_printf(P_INFO2, "[WebData_free] freed data->response->data\n");
			}
			free(data->response);
			dbg_printf(P_INFO2, "[WebData_free] freed data->response\n");
		}
		if(data->header){
			if(data->header->data) {
				free(data->header->data);
				dbg_printf(P_INFO2, "[WebData_free] freed data->header->data\n");
			}
			free(data->header);
			dbg_printf(P_INFO2, "[WebData_free] freed data->header\n");
		}
		free(data);
		dbg_printf(P_INFO2, "[WebData_free] freed data\n");
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
	dbg_printf(P_INFO, "[getHTTPData] response code: %ld\n", rc);
	curl_easy_cleanup(curl_handle);
	if(rc != 200) {
		dbg_printf(P_ERROR, "[getHTTPData] Failed to get '%s' [response: %ld]\n", url, rc);
		WebData_free(data);
		return NULL;
	} else {
		data->header->data = realloc(data->header->data, data->header->size + 1);
		if(data->header->data)
			dbg_printf(P_INFO2, "[getHTTPData] reallocated data->header->data to %ld bytes\n", data->header->size + 1);
		data->response->data = realloc(data->response->data, data->response->size + 1);
		if(data->response->data)
			dbg_printf(P_INFO2, "[getHTTPData] reallocated data->response->data to %ld bytes\n", data->response->size + 1);
		return data;
	}
}

