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
#include "json.h"

/** \brief Create a Transmission-specific JSON packet in order to add a new download
 * to Transmission.
 *
 * \param[in] torrent_name A filename, a URL or a magnet link
 * \param[in] start Determines if the torrent shall start to download right away or be added in a paused state
 * \param[in] folder Optional parameter to set the download folder for this torrent
 * \param[out] setme_size size of the resulting JSON packet
 * \return pointer to the JSON string
 *
 * The function Base64-encodes the given torrent content and encapsulates it in a JSON packet.
 * The packet can then be sent to Transmission via HTTP POST.
 */
char* makeTorrentAddFilenameJSON(const char* torrent_name, uint8_t start, const char* folder, uint32_t *setme_size) {
  char *buf = NULL;
  char *folder_str = NULL;
  int buf_size, json_size, folderstr_size = 0;
  const char *JSONstr =
    "{\n"
    "\"method\": \"torrent-add\",\n"
    "\"arguments\": {\n"
    "\"filename\": \"%s\",\n"
    "%s"
    "\"paused\": %d\n"
    "}\n"
    "}";

  if(!setme_size) {
    dbg_printf(P_ERROR, "setme_size == NULL");
    return NULL;
  }

  *setme_size = 0;

  if(folder && *folder) {
    folderstr_size = strlen(folder) + 20;
    folder_str = (char*)am_malloc(folderstr_size);
    assert(folder_str && "am_malloc(folder_str) failed!");
    snprintf(folder_str, folderstr_size, "\"download-dir\": \"%s\",\n", folder);
    dbg_printf(P_INFO, "folder_str: %s", folder_str);
  }

  buf_size = strlen(torrent_name) + strlen(JSONstr) + folderstr_size + 10;
  buf = (char*)am_malloc(buf_size);
  memset(buf, 0, buf_size);

  json_size = snprintf(buf, buf_size, JSONstr, torrent_name, folder_str ? folder_str : "", start ? 0 : 1);

  if(json_size < 0 || json_size >= buf_size) {
    dbg_printf(P_ERROR, "Error producing JSON string with Base64-encoded metadata: %s", strerror(errno));
    am_free(folder_str);
    am_free(buf);
    return NULL;
  }

  buf[json_size] = '\0';
  dbg_printf(P_INFO2, "JSON: %s", buf);

  *setme_size = json_size;

  am_free(folder_str);
  return buf;
}

/** \brief Create a Transmission-specific JSON packet in order to add a new download
 * to Transmission.
 *
 * \param[in] data Pointer to the torrent data
 * \param[in] tsize size of the torrent data
 * \param[in] start Determines if the torrent shall start to download right away or be added in a paused state
 * \param[in] folder Optional parameter to set the download folder for this torrent
 * \param[out] setme_size size of the resulting JSON packet
 * \return pointer to the JSON string
 *
 * The function Base64-encodes the given torrent content and encapsulates it in a JSON packet.
 * The packet can then be sent to Transmission via HTTP POST.
 */
char* makeTorrentAddMetaInfoJSON(const void *data, uint32_t tsize, uint8_t start, const char* folder, uint32_t *setme_size) {

  char *encoded = NULL;

  char *buf = NULL;
   char *folder_str = NULL;
  int buf_size, json_size, folderstr_size = 0;
  uint32_t enc_size;
  const char *JSONstr =
    "{\n"
    "\"method\": \"torrent-add\",\n"
    "\"arguments\": {\n"
    "\"metainfo\": \"%s\",\n"
      "%s"
      "\"paused\": %d\n"
    "}\n"
    "}";

  if(!setme_size) {
    dbg_printf(P_ERROR, "setme_size == NULL");
    return NULL;
  }

  *setme_size = 0;

  encoded = base64_encode((const char*)data, tsize, &enc_size);

  if(encoded && enc_size > 0) {
    if(folder && *folder) {
      folderstr_size = strlen(folder) + 20;
      folder_str = (char*)am_malloc(folderstr_size);
      assert(folder_str && "am_malloc(folder_str) failed!");
      snprintf(folder_str, folderstr_size, "\"download-dir\": \"%s\",\n", folder);
      dbg_printf(P_INFO, "folder_str: %s", folder_str);
    }

    buf_size = enc_size + strlen(JSONstr) + folderstr_size + 10;
    buf = (char*)am_malloc(buf_size);
    memset(buf, 0, buf_size);
    json_size = snprintf(buf, buf_size, JSONstr, encoded, folder_str ? folder_str : "", start ? 0 : 1);
    if(json_size < 0 || json_size >= buf_size) {
      dbg_printf(P_ERROR, "Error producing JSON string with Base64-encoded metadata: %s", strerror(errno));
      am_free(folder_str);
      am_free(encoded);
      am_free(buf);
      return NULL;
    }

    buf[json_size] = '\0';
    dbg_printf(P_INFO2, "JSON: %s", buf);

    *setme_size = json_size;

    am_free(folder_str);
    am_free(encoded);
    return buf;
  }

  return NULL;
}

char* makeChangeUpSpeedJSON(torrent_id_t tID, uint32_t upspeed, uint8_t rpcVersion, uint32_t *setme_size) {
  char *buf = NULL;
  int buf_size, json_size = 0;
  const char *JSONstr =
     "{\n"
     "\"method\": \"torrent-set\",\n"
     "\"arguments\": {\n"
     "\"ids\": %d,\n"
     "\"%s\": %d,\n"
     "\"%s\": true\n"
     "}\n"
     "}";

  if(!setme_size) {
    dbg_printf(P_ERROR, "setme_size == NULL");
    return NULL;
  }

  *setme_size = 0;

  if(rpcVersion <= 0) {
    dbg_printf(P_ERROR, "Invalid RPC version: %d", rpcVersion);
    return NULL;
  }

  if(upspeed <= 0) {
    dbg_printf(P_ERROR, "Invalid upspeed value: %d", upspeed);
    return NULL;
  }

  if(tID <= 0) {
    dbg_printf(P_ERROR, "Invalid torrent ID: %d", tID);
    return NULL;
  }

  buf_size = strlen(JSONstr) + 60;
  buf = am_malloc(buf_size);
  if(!buf) {
    dbg_printf(P_DBG, "Mem alloc for JSON string failed!");
    return NULL;
  }
  memset(buf, 0, buf_size);

  if(rpcVersion <= 4) {
    json_size = snprintf(buf, buf_size, JSONstr, tID, "speed-limit-up", upspeed, "speed-limit-up-enabled");
  } else if(rpcVersion >= 5 ) {
    json_size = snprintf(buf, buf_size, JSONstr, tID, "uploadLimit", upspeed, "uploadLimited");
  }

  if(json_size < 0 || json_size >= buf_size) {
    dbg_printf(P_ERROR, "Error producing JSON string with Base64-encoded metadata: %s", strerror(errno));
    am_free(buf);
    return NULL;
  }
  buf[json_size] = '\0';

  *setme_size = json_size;

  return buf;
}

/** \brief Parse the JSON-encoded response from Transmission after uploading a torrent via JSON-RPC.
 *
 * \param[in] response Response from Transmission
 * \return Result message from Transmission in form of a string
 *
 * The JSON response contains a field "result" with a string value. This value is extracted and returned.
 * Possible values are "success", "duplicate torrent" or failure messages.
 */

char* parseResponse(const char* response) {
  const char* result_regex = "\"result\":\\s*\"(.+)\"";
  return getRegExMatch(result_regex, response, 1);
}

torrent_id_t parseTorrentID(const char* response) {
  const char* result_regex = "\"id\":\\s*(\\d+)";
  char *result_str = NULL;
  torrent_id_t id = -1;
  result_str = getRegExMatch(result_regex, response, 1);
  if(result_str) {
   id = atoi(result_str);
   am_free(result_str);
  }

  return id;
}


int8_t parseRPCVersion(const char* response) {
  const char* result_regex = "\"rpc-version\":\\s*(\\d+)";
  char *result_str = NULL;
  int8_t result = -1;
  result_str = getRegExMatch(result_regex, response, 1);

  if(result_str) {
   result = atoi(result_str);
   am_free(result_str);
  }

  return result;
}
