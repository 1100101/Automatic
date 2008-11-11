/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file json.c
 *
 * JSON encoder (Transmission-specific)
 * \internal Created on: Oct 13, 2008
 *  Author: Frank Aurich
 */

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <pcre.h>
#include <string.h>
#include <assert.h>

#include "base64.h"
#include "output.h"
#include "utils.h"

/** \brief Create a Transmission-specific JSON packet in order to add a new download
 * to Transmission.
 *
 * \param[in] data Pointer to the torrent data
 * \param[in] tsize size of the torrent data
 * \param[out] setme_size size of the resulting JSON packet
 * \return pointer to the JSON string
 *
 * The function Base64-encodes the given torrent content and encapsulates it in a JSON packet.
 * The packet can then be sent to Transmission via HTTP POST.
 */
char* makeJSON(const void *data, uint32_t tsize, uint32_t *setme_size) {

	char *encoded = NULL;

	char *buf = NULL;
	int buf_size, json_size;
	uint32_t enc_size;
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

/** \brief Parse the JSON-encoded response from Transmission after uploading a torrent via JSON-RPC.
 *
 * \param[in] response Response from Transmission
 * \return Result message from Transmission in form of a string
 *
 * The JSON response contains a field "result" with a string value. This value is extracted and returned.
 * Possible values are "success", "duplicate torrent" or failure messages.
 */

#define OVECCOUNT 30
const char* parseResponse(const char* response) {
	int err, len;
	int erroffset;
	const char *error = NULL;
	const char* result_regex = "\"result\": \"(.+)\"\n";
	pcre *result_preg = NULL;
	char *result_str = NULL;
	int ovector[OVECCOUNT];

	result_preg = pcre_compile(result_regex, PCRE_CASELESS|PCRE_EXTENDED,
											&error,               /* for error message */
											&erroffset,           /* for error offset */
											NULL);                /* use default character tables */

	if(result_preg == NULL) {
		dbg_printf(P_ERROR, "PCRE compilation failed at offset %d: %s\n", erroffset, error);
	} else {
		err = pcre_exec(result_preg, NULL, response, strlen(response),
										0, 0, ovector, OVECCOUNT);
		if(err >= 0) { /* regex matches */
			assert(err == 1);
			len = ovector[1] - ovector[0];
			dbg_printf(P_DBG, "result len: %d (end: %d, start: %d)", len, ovector[1], ovector[0]);
			result_str = am_strndup(response + ovector[0], len);
		} else {
			if(err == PCRE_ERROR_NOMATCH) {
				dbg_printf(P_INFO, "[parseResponse] regexec didn't match. Response was: %s", response);
			} else {
				dbg_printf(P_ERROR, "[parseResponse] regexec error: %d", err);
			}
		}
		pcre_free(result_preg);
	}
	return result_str;
}
