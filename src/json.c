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
#include <string.h>
#include <assert.h>

#include "base64.h"
#include "output.h"
#include "regex.h"
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
char* makeJSON(const void *data, uint32_t tsize, uint8_t start, uint32_t *setme_size) {

	char *encoded = NULL;

	char *buf = NULL;
	int buf_size, json_size;
	uint32_t enc_size;
	const char *JSONstr =
		"{\n"
		"\"method\": \"torrent-add\",\n"
		"\"arguments\": {\n"
		"\"metainfo\": \"%s\",\n"
    "\"paused\": %d\n"
		"}\n"
		"}";

  *setme_size = 0;

  encoded = base64_encode(data, tsize, &enc_size);

  if(encoded && enc_size > 0) {
    buf_size = enc_size + strlen(JSONstr) + 10;
    buf = am_malloc(buf_size);
    memset(buf, 0, buf_size);
    json_size = snprintf(buf, buf_size, JSONstr, encoded, start ? 0 : 1);
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

const char* parseResponse(const char* response) {
	const char* result_regex = "\"result\":\\s+\"(.+)\"";
	char *result_str = NULL;
	result_str = getRegExMatch(result_regex, response, 1);
	return result_str;
}
