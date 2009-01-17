/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file web.c
 *
 * Provides basic functionality for communicating with HTTP and FTP servers.
 */

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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <curl/curl.h>
#include <stdint.h>

#include "web.h"
#include "output.h"
#include "regex.h"
#include "utils.h"


/** \cond */
#define DATA_BUFFER 1024 * 100
#define HEADER_BUFFER 500
/** \endcond */

static size_t write_header_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	WebData *mem = data;
	char *buf = NULL;
	char *tmp = NULL;
	char *filename = NULL;
	const char* content_pattern = "Content-Disposition:\\s(inline|attachment);\\s+filename=\"(.+)\"$";
	int content_length = 0;
	uint32_t i;


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
	dbg_printf(P_INFO2, "%s", buf);

	/* parse header for Content-Length to allocate correct size for data->response->data */
	if(!strncmp(buf, "Content-Length:", 15)) {
		tmp = getRegExMatch("Content-Length:\\s(\\d+)", buf, 1);
		if(tmp != NULL) {
			dbg_printf(P_INFO2, "Content-Length: %s", tmp);
			content_length = atoi(tmp);
			if(content_length > 0) {
				mem->content_length = content_length;
				mem->response->data = am_realloc(mem->response->data, content_length + 1);
			}
			am_free(tmp);
		}
	} else {
		/* parse header for Content-Disposition to get correct filename */
		if(!strncmp(buf, "Content-Disposition", 19)) {
			filename = getRegExMatch(content_pattern, buf, 2);
			if(filename) {
				mem->content_filename = filename;
				dbg_printf(P_INFO2, "[write_header_callback] Found filename: %s", mem->content_filename);
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
	HTTPData* data = NULL;

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


/** \brief Create a new WebData object
 *
 * \param[in] url URL for a WebData object
 *
 * The parameter \a url is optional. You may provide \c NULL if no URL is required or not known yet.
 */
struct WebData* WebData_new(const char *url) {
	WebData *data = NULL;

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


/** \brief Free a WebData object and all memory associated with it
 *
 * \param[in] data Pointer to a WebData object
 */
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

/** \brief Download data from a given URL
 *
 * \param[in] url URL of the object to download
 * \return a WebData object containing the complete response as well as header information
 *
 * getHTTPData() attempts to download the file pointed to by \a url and stores the content in a WebData object.
 * The function returns \c NULL if the download failed.
 */

WebData* getHTTPData(const char *url) {
	CURL *curl_handle = NULL;
	CURLcode res;
	char errorBuffer[CURL_ERROR_SIZE];
	WebData* data = NULL;
	long rc;

	if(!url) {
		return NULL;
	}

	data = WebData_new(url);
	if(!data)
		return NULL;

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errorBuffer);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
 	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, getenv( "AM_CURL_VERBOSE" ) != NULL);
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


/** \brief Upload data to a specified URL.
 *
 * \param url Path to where data shall be uploaded
 * \param auth (Optional) authentication information in the form of "user:password"
 * \param data Data that shall be uploaded
 * \param data_size size of the data
 * \return Web server response
 */
char *sendHTTPData(const char *url, const char* auth, const void *data, uint32_t data_size) {
	CURL *curl_handle = NULL;
	CURLcode res;
	long rc;
	struct curl_slist *headers = NULL;
	WebData* response_data = NULL;
	char *ret = NULL;

	if(!data || !url) {
	  return NULL;
	}


	response_data = WebData_new(url);

	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_URL, response_data->url);

	if(auth) {
		dbg_printf(P_INFO2, "auth: %s", auth);
		curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
		curl_easy_setopt(curl_handle, CURLOPT_USERPWD, auth);
	}

	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data_size);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data_callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, response_data);
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, getenv( "AM_CURL_VERBOSE" ) != NULL);

	res = curl_easy_perform(curl_handle);

	if(res != 0) {
			dbg_printf(P_ERROR, "[uploadData] Failed to upload to '%s': %s", url, curl_easy_strerror(res));
			ret = NULL;
	} else {
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &rc);
		dbg_printf(P_INFO2, "[uploadData] response code: %ld", rc);
		if(rc != 200) {
			dbg_printf(P_ERROR, "[uploadData] Failed to upload to '%s' [response: %ld]", url, rc);
			ret = NULL;
		} else {
			ret = am_strndup(response_data->response->data, response_data->response->size);
		}
	}
	curl_easy_cleanup(curl_handle);
	curl_slist_free_all(headers);
	WebData_free(response_data);
	return ret;
}

