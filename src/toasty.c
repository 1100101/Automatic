/*
 * Copyright (C) 2008 Frank Aurich (1100101+automatic@gmail.com
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


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#include "toasty.h"
#include "web.h"
#include "output.h"
#include "utils.h"

#define TOASTY_URL "http://api.supertoasty.com"
#define TOASTY_ADD "/notify"

static const char* getToastyErrorMessage(const uint16_t responseCode) {
  const char* response;

  switch(responseCode) {
    case 400:
      response = "Bad request";
      break;
    case 401:
      response = "Invalid API key";
      break;
    case 405:
      response = "Method not allowed (non-SSL connection)";
      break;
    case 406:
      response = "API limit exceeded";
      break;
    case 500:
      response = "Internal server error";
      break;
    default:
      response = "Unknown error!";
  }
  return response;
}

static char* createToastyMessage(const char* event, const char* desc, int32_t *size) {
  int32_t result, total_size;
  char *msg = NULL;

  assert(size != NULL);

  *size = 0;

  if(!event) {
    dbg_printf(P_ERROR, "[createToastyMessage] event == NULL");
    *size = 0;
    return NULL;
  }

  if(!desc) {
    dbg_printf(P_ERROR, "[createToastyMessage] desc == NULL");
    *size = 0;
    return NULL;
  }

  total_size = strlen(event) + strlen(desc) + 80;
  msg = (char*)am_malloc(total_size);

  if(msg) {
    result = snprintf(msg, total_size, "sender=Automatic&title=%s&text=%s", event, desc);
    *size = result;
  }
  return msg;
}

int16_t sendToastyNotification(const char* deviceid, const char* event, const char* desc) {
  int16_t        result = -1;
  int32_t       data_size;
  char          url[128];
  HTTPResponse *response = NULL;
  char         *data = NULL;

  if(!deviceid) {
    dbg_printf(P_ERROR, "[sendToastyNotification] deviceid == NULL");
    return -1;
  }

  data = createToastyMessage(event, desc, &data_size);

  if(data) {
    snprintf(url, 128, "%s%s/%s", TOASTY_URL, TOASTY_ADD, deviceid);
    response = sendHTTPData(url, NULL, data, data_size);
    if(response) {
      if(response->responseCode == 200) {
        result = 1;
      } else {
        dbg_printf(P_ERROR, "Toasty Notification failed: %s (%d)",
              getToastyErrorMessage(response->responseCode),
              response->responseCode);
        result = -response->responseCode;
      }
      HTTPResponse_free(response);
    }
    am_free(data);
  }

  return result;
}

int8_t toasty_sendNotification(enum prowl_event event, const char* deviceid, const char *filename) {
  int8_t result;
  char desc[500];
  char *event_str = NULL;

  switch(event) {
    case PROWL_NEW_DOWNLOAD:
      event_str = "Torrent File Auto-Added";
      snprintf(desc, sizeof(desc), "%s", filename);
      break;
    case PROWL_DOWNLOAD_FAILED:
      event_str = "Auto-Add Failed";
      snprintf(desc, sizeof(desc), "%s", filename);
      break;
    default:
      dbg_printf(P_ERROR, "Unknown Toasty event code %d", event);
      return 0;
  }

  dbg_printf(P_INFO, "[toasty_sendNotification] I: %d E: %s\tD: %s", event, event_str, desc);

  if(sendToastyNotification(deviceid, event_str, desc) == 1) {
    result = 1;
  } else {
    result = 0;
  }
  return result;
}
