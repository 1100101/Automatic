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

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#include "prowl.h"
#include "web.h"
#include "output.h"
#include "utils.h"

#define PROWL_URL "https://prowl.weks.net"
#define PROWL_ADD "/publicapi/add"
#define PROWL_VERIFY "/publicapi/verify"

static const char* getProwlErrorMessage(const uint16_t responseCode) {
  const char* response;

  switch(responseCode) {
    case 400:
      response = "Bad request";
      break;
    case 401:
      response = "Invald API key";
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

static char* createProwlMessage(const char* apikey, const char* event, const char* desc, int32_t *size) {
  int32_t result, apikey_length, event_length, desc_length, total_size;

  char *msg = NULL;

  *size = 0;

  if(!apikey) {
    dbg_printf(P_ERROR, "[createProwlMessage] apikey == NULL");
    *size = 0;
    return NULL;
  }

  if((!event && !desc)) {
    dbg_printf(P_ERROR, "[createProwlMessage] event == NULL && desc == NULL");
    *size = 0;
    return NULL;
  }

  apikey_length = strlen(apikey);
  event_length  = event ? strlen(event) : 0;
  desc_length   = desc  ? strlen(desc)  : 0;

  total_size = apikey_length + event_length + desc_length + 80;
  msg = (char*)am_malloc(total_size);

  if(msg) {
    result = snprintf(msg, total_size, "apikey=%s&priority=0&application=Automatic&event=%s&description=%s",
        apikey, event, desc);
    *size = result;
  }
  return msg;
}
int16_t sendProwlNotification(const char* apikey, const char* event, const char* desc) {
  int16_t        result = -1;
  int32_t       data_size;
  char          url[128];
  HTTPResponse *response = NULL;
  char         *data = NULL;

  data = createProwlMessage(apikey, event, desc, &data_size);

  if(data) {
    snprintf(url, 128, "%s%s", PROWL_URL, PROWL_ADD);
    response = sendHTTPData(url, NULL, data, data_size);
    if(response) {
      if(response->responseCode == 200) {
        result = 1;
      } else {
        dbg_printf(P_ERROR, "Prowl Notification failed: %s (%d)",
              getProwlErrorMessage(response->responseCode),
              response->responseCode);
        result = -response->responseCode;
      }
      HTTPResponse_free(response);
    }
    am_free(data);
  }
  return result;
}

int16_t verifyProwlAPIKey(const char* apikey) {

  int16_t result = -1;
  char url[128];
  HTTPResponse *response = NULL;

  if(apikey) {
    snprintf(url, 128, "%s%s?apikey=%s", PROWL_URL, PROWL_VERIFY, apikey);
    response = getHTTPData(url);
    if(response) {
      if(response->responseCode == 200) {
        dbg_printf(P_INFO, "Prowl API key '%s' is valid", apikey);
        result = 1;
      } else {
        dbg_printf(P_ERROR, "Error: Prowl API  key '%s' is invalid!", apikey);
        result = -response->responseCode;
      }
      HTTPResponse_free(response);
    }
  }
  return result;
}


int8_t prowl_sendNotification(enum prowl_event event, const char* apikey, const char *filename) {
  int8_t result;
  char desc[500];
  char *event_str = NULL;

  switch(event) {
    case PROWL_NEW_DOWNLOAD:
      event_str = "Torrent File Auto-Added";
      snprintf(desc, 500, "%s", filename);
      break;
    case PROWL_DOWNLOAD_FAILED:
      event_str = "Auto-Add Failed";
      snprintf(desc, 500, "%s", filename);
      break;
    default:
      dbg_printf(P_ERROR, "Unknown Prowl event code %d", event);
      return 0;
  }

  dbg_printf(P_INFO2, "[prowl_sendNotification] I: %d E: %s\tD: %s", event, event_str, desc);

  if(sendProwlNotification(apikey, event_str, desc) == 1) {
    result = 1;
  } else {
    result = 0;
  }
  return result;
}
