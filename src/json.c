/*
 * json.c
 *
 *  Created on: Oct 13, 2008
 *      Author: aurich
 */


#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "base64.h"
#include "output.h"
#include "utils.h"

char* makeJSON(const void *data, uint32_t tsize, uint32_t *setme_size) {

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
