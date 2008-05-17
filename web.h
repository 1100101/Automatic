#ifndef WEB_H__
#define WEB_H__

#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>
#include "memwatch.h"

struct HTTPData {
 char *data;
 size_t size;
};

struct WebData {
	char *url;
	struct HTTPData* header;
	struct HTTPData* response;
};


typedef struct HTTPData HTTPData;
typedef struct WebData WebData;

WebData* getHTTPData(const char *url);
void WebData_free(struct WebData *data);


#endif
