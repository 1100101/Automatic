#ifndef WEB_H__
#define WEB_H__

struct HTTPData {
 char *data;
 size_t size;
};

struct WebData {
	char *url;
	long responseCode;
	size_t content_length;
	char *content_filename;
	struct HTTPData* header;
	struct HTTPData* response;
};


typedef struct HTTPData HTTPData;
typedef struct WebData WebData;

WebData* getHTTPData(const char *url);
void WebData_free(struct WebData *data);
void cd_preg_free(void);

#endif
