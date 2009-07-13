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


#include "prowl.h"
#include "web.h"
#include "output.h"
#include "utils.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <stdlib.h>
#include <stdio.h>

#define PROWL_URL "https:\/\/prowl.weks.net"
#define PROWL_ADD "/publicapi/add"
#define PROWL_VERIFY "/publicapi/verify"

static char* createProwlMessage(const char* apikey, const char* event, const char* desc, int32_t *size) {
  int32_t result;

  char *msg = NULL;

  *size = 0;

  if(!apikey || (!event && !desc)) {
    *size = 0;
    return NULL;
  }

  int32_t apikey_length = strlen(apikey);
  int32_t event_length  = strlen(event);
  int32_t desc_length   = strlen(desc);

  msg = (char*)am_malloc(apikey_length + event_length + desc_length + 80);

  if(msg) {
    result = snprintf(msg, max_size, "apikey=%s&priority=0&application=Automatic&event=%s&description=%s",
        apikey, event, description);
    *size = result;
  }
  return msg;
}

int8_t sendProwlNotification(const char* apikey, const char* event, const char* desc) {
  int32_t data_size;
  char url[128];
  char *response = NULL;
  char *data = NULL;

  data = createProwlMessage(apikey, event, desc, &data_size);

  if(data) {
    snprintf(url, 128, "%s%s", PROWL_URL, PROWL_ADD);
    response = sendHTTPData(url, NULL, data, data_size);
  }

  am_free(response);
  am_free(data);
}

int8_t verifyProwlAPIKey(const char* apikey) {

  int8_t result;
  char url[128];
  WebData *response = NULL;

  if(apikey) {
    snprintf(url, 128, "%s%s&apikey=%s", PROWL_URL, PROWL_ADD, apikey);
//    response = sendHTTPData(url, NULL, data, data_size);
    response = getHTTPData(url);
  }

  WebData_free(response);
  return result;
}


