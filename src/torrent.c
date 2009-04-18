/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file torrent.c
 *
 * Not a clue.
 *
 * \internal Created on: Oct 13, 2008
 * Author: Frank Aurich
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>


#include "output.h"
#include "utils.h"
#include "json.h"
#include "web.h"
#include "file.h"

static int is_torrent(const char *str) {
  if(strstr(str, ".torrent"))
  return 1;
  else
  return 0;
}


/** \brief Determine the filename of a downloaded torrent and create its save path.
 *
 * \param path Full path where the torrent will be saved
 * \param content_filename filename as determined by the webserver in the header ("Content-Disposition")
 * \param url URL of a downloaded torrent
 * \param t_folder Full path to the torrent folder
 *
 * The function first tries to find a filename for a downloaded torrent. If the webserver sent
 * a concrete name via its header ("Content-Disposition: attachment; filename=..."), that is used.
 * If the webserver didn't send a filename, the function uses the last part of the torrent URL
 * to create a filename. The ".torrent" extension is appended if necessary.
 * The resulting filename is then appended to the torrent folder path (specified in automatic.conf)
 */
void get_filename(char *path, const char *content_filename, const char* url, const char *t_folder) {
  char *p, tmp[PATH_MAX], buf[PATH_MAX];
  int len;


#ifdef DEBUG
  assert(url);
  assert(t_folder);
#endif

  if (content_filename) {
    strncpy(buf, content_filename, strlen(content_filename) + 1);
  } else {
    strcpy(tmp, url);
    p = strtok(tmp, "/");
    while (p) {
      len = strlen(p);
      if (len < PATH_MAX)
        strcpy(buf, p);
      p = strtok(NULL, "/");
    }
  }
  snprintf(path, PATH_MAX - 1, "%s/%s%s", t_folder, buf, (is_torrent(buf) ? "" : ".torrent"));
}

static int8_t changeUploadSpeed(const char* url, const char* auth, int8_t torrentID, uint16_t upspeed) {

  uint32_t packet_size;
  char *packet = NULL;
  char *res = NULL;
  const char *response = NULL;
  uint8_t result = 0;


  if(torrentID > -1) {
    packet = makeChangeUpSpeedJSON(torrentID, upspeed, &packet_size);
    if(packet && packet_size > 0) {
      am_free(res);
      res = sendHTTPData(url, auth, packet, packet_size);
      if(res != NULL) {
        response = parseResponse(res);
        if(response) {
          if(!strncmp(response, "success", 7)) {
            dbg_printf(P_MSG, "Uplimit successfully changed!");
            result = 1;
          }
          am_free((void*)response);
        }
        am_free(res);
      }
      am_free(packet);
    }
  }
  return result;
}

int8_t uploadTorrent(const void *t_data, int t_size,
                     const char *host, const char* auth, uint16_t port,
                     int16_t upspeed, uint8_t start) {
  char *packet = NULL, *res = NULL;
  const char *response = NULL;
  uint32_t packet_size = 0, ret = -1;
  char url[MAX_URL_LEN];

  /* packet torrent data in a JSON package */
  packet = makeJSON(t_data, t_size, start, &packet_size);
  if(packet && packet_size > 0) {
    snprintf( url, MAX_URL_LEN, "http://%s:%d/transmission/rpc", host, port);
    /* send JSON package to Transmission via HTTP POST */
    res = sendHTTPData(url, auth, packet, packet_size);
    if(res != NULL) {
      response = parseResponse(res);
      if(!strncmp(response, "success", 7)) {
        dbg_printf(P_MSG, "Torrent upload successful!");
        if(upspeed > 0) {
          changeUploadSpeed(url, auth, getTorrentID(res), upspeed);
        }
        ret = 0;
      } else if(!strncmp(response, "duplicate torrent", 17)) {
        dbg_printf(P_MSG, "Torrent has already been added to Transmission");
        ret = 0;
      } else {
        dbg_printf(P_ERROR, "Error uploading torrent: %s", response);
        ret = -1;
      }
      am_free((void*)response);
      am_free(res);
    }
    am_free(packet);
  }
  return ret;
}
