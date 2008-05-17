#include "web.h"
#include "output.h"

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

struct WebData* WebData_new(const char *url) {
	WebData *data;
	int len;

	data = malloc(sizeof(WebData));
	dbg_printf(P_INFO2, "[WebData_new] allocated %d bytes for wData\n", sizeof(WebData));
	if(url) {
		len = strlen(url);
		data->url = malloc(len + 1);
		strncpy(data->url, url, len);
		data->url[len] = '\0';
	}
	data->header = malloc(sizeof(HTTPData));
	dbg_printf(P_INFO2, "[WebData_new] allocated %d bytes for data->header\n", sizeof(HTTPData));
	data->header->data = NULL;
	data->header->size = 0;
	data->response = malloc(sizeof(HTTPData));
	dbg_printf(P_INFO2, "[WebData_new] allocated %d bytes for data->response\n", sizeof(HTTPData));
	data->response->data = NULL;
	data->response->size = 0;
	return data;
}

void WebData_free(struct WebData *data) {
	if(data) {
		if(data->url) {
			free(data->url);
		}
		if(data->response) {
			if(data->response->data) {
				free(data->response->data);
			}
			free(data->response);
		}
		if(data->header){
			if(data->header->data) {
				free(data->header->data);
			}
			free(data->header);
		}
		free(data);
		data = NULL;
	}
}

WebData* getHTTPData(const char *url) {
	CURL *curl_handle;
	CURLcode res;
	char errorBuffer[CURL_ERROR_SIZE];

	WebData* data = WebData_new(url);

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, errorBuffer);
 	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, data->header);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, data->response);
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	res = curl_easy_perform(curl_handle);
	curl_easy_cleanup(curl_handle);
	if (res != CURLE_OK) {
		fprintf(stderr, "Failed to get '%s' [%s]\n", url, errorBuffer);
		return NULL;
	} else {
		return data;
	}
}

/*
int main(void) {
	const char* url = "http://www.mininova.org/get/1410744";
	WebData *data = getHTTPData(url);
	printf("Response size:\n%d\n", data->response->size);
	get_filename(data);
	WebData_free(data);
	return 0;
}


*/
