/* $Id$
* $Name$
* $ProjectName$
*/

/**
* @file web.c
*
* Provides basic functionality for communicating with HTTP and FTP servers.
*/

/*
* Copyright (C) 2008 Frank Aurich (1100101+automatic@gmail.com)
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <curl/curl.h>
#include <stdint.h>

#include "web.h"
#include "output.h"
#include "regex.h"
#include "urlcode.h"
#include "utils.h"


/** \cond */
#define DATA_BUFFER 1024 * 100
#define HEADER_BUFFER 500
/** \endcond */

PRIVATE char *gSessionID = NULL;

PRIVATE uint8_t gbGlobalInitDone = FALSE;

/** Generic struct storing data and the size of the contained data */
typedef struct HTTPData {
  /** \{ */
 char   *data;  /**< Stored data */
 size_t  size;  /**< Size of the stored data */
 /** \{ */
} HTTPData;

/** Struct storing information about data downloaded from the web */
typedef struct WebData {
  /** \{ */
  char      *url;              /**< URL of the WebData object */
  long       responseCode;     /**< HTTP response code        */
  size_t     content_length;   /**< size of the received data determined through header field "Content-Length" */
  char      *content_filename; /**< name of the downloaded file determined through header field "Content-Length" */
  //HTTPData  *header;           /**< complete header information in a HTTPData object */
  HTTPData  *response;         /**< HTTP response in a HTTPData object */
  /** \} */
} WebData;


PUBLIC void SessionID_free(void) {
   am_free(gSessionID);
   gSessionID = NULL;
}

PRIVATE size_t write_header_callback(void *ptr, size_t size, size_t nmemb, void *data) {
  size_t       line_len = size * nmemb;
  WebData     *mem  = (WebData*)data;
  const char  *line = (const char*)ptr;
  char        *tmp  = NULL;
  char        *filename = NULL;
  const char  *content_pattern = "Content-Disposition:\\s(inline|attachment);\\s+filename=\"?(.+?)\"?;?\\r?\\n?$";
  int          content_length = 0;

  /* parse header for Content-Length to allocate correct size for data->response->data */
  if(line_len >= 15 && !memcmp(line, "Content-Length:", 15)) {
    tmp = getRegExMatch("Content-Length:\\s(\\d+)", line, 1);
    if(tmp != NULL) {
      dbg_printf(P_INFO2, "Content-Length: %s", tmp);
      content_length = atoi(tmp);
      if(content_length > 0) {
        mem->content_length = content_length;
        mem->response->data = am_realloc(mem->response->data, content_length + 1);
      }
      am_free(tmp);
    }
  } else if(line_len >= 19 && !memcmp(line, "Content-Disposition", 19)) {
    /* parse header for Content-Disposition to get correct filename */
    filename = getRegExMatch(content_pattern, line, 2);
    if(filename) {
      mem->content_filename = filename;
      dbg_printf(P_INFO2, "[write_header_callback] Found filename: %s", mem->content_filename);
    }
  }

  //not sure if the following part is really necessary
#if 0
  /* save header line to mem->header */
  if(!mem->header->data) {
    mem->header->data = am_malloc(HEADER_BUFFER);
    dbg_printf(P_INFO2, "[write_header_callback] allocated %d bytes for mem->header->data", HEADER_BUFFER);
  }


  if(mem->header->size + line_len + 1 > HEADER_BUFFER) {
    mem->header->data = (char *)am_realloc(mem->header->data, mem->header->size + line_len + 1);
  }

  if (mem->header->data) {
    memcpy(&(mem->header->data[mem->header->size]), ptr, line_len);
    mem->header->size += line_len;
    mem->header->data[mem->header->size] = 0;
  }

#endif

  return line_len;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE size_t parse_Transmission_response(void *ptr, size_t size, size_t nmemb, void *data) {
  size_t        line_len = size * nmemb;
  const char   *line = ptr;
  WebData      *mem = data;
  const char   *session_key = "X-Transmission-Session-Id: ";
  const size_t  key_len = strlen( session_key );
  char         *tmp = NULL;
  int           content_length = 0;


  if( (line_len >= key_len) && !memcmp(line, session_key, key_len) ) {
      const char * begin = line + key_len;
      const char * end = begin;
      while( !isspace( *end ) ) {
        ++end;
      }
      am_free( gSessionID );
      gSessionID = NULL;
      gSessionID = am_strndup( begin, end-begin );
  /* parse header for Content-Length to allocate correct size for data->response->data */
  } else if(line_len >= 15 && !memcmp(line, "Content-Length:", 15)) {
    tmp = getRegExMatch("Content-Length:\\s(\\d+)", line, 1);
    if(tmp != NULL) {
      dbg_printf(P_INFO2, "Content-Length: %s", tmp);
      content_length = atoi(tmp);
      if(content_length > 0) {
        mem->content_length = content_length;
        mem->response->data = am_realloc(mem->response->data, content_length + 1);
      }
      am_free(tmp);
    }
  }

  return line_len;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE size_t write_data_callback(void *ptr, size_t size, size_t nmemb, void *data) {
  size_t line_len = size * nmemb;
  WebData *mem = data;

  /**
   * if content-length detection in write_header_callback was not successful, mem->response->data will be NULL
   * as a fallback, allocate a predefined size of memory and realloc if necessary
  **/
  if(!mem->response->data) {
    mem->response->data = (char*)am_malloc(DATA_BUFFER);
    dbg_printf(P_INFO2, "[write_data_callback] allocated %d bytes for mem->response->data", DATA_BUFFER);
  }

  if(mem->response->size + line_len + 1 > DATA_BUFFER) {
    mem->response->data = (char *)am_realloc(mem->response->data, mem->response->size + line_len + 1);
  }

  if (mem->response->data) {
    memcpy(&(mem->response->data[mem->response->size]), ptr, line_len);
    mem->response->size += line_len;
    mem->response->data[mem->response->size] = 0;
  }

  return line_len;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE struct HTTPData* HTTPData_new(void) {
  HTTPData* data = NULL;

  data = am_malloc(sizeof(struct HTTPData));
  if(!data) {
    return NULL;
  }
  data->data = NULL;
  data->size = 0;
  return data;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE void HTTPData_free(HTTPData* data) {

  if(data)
    am_free(data->data);
  am_free(data);
  data = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/** \brief Free a WebData object and all memory associated with it
*
* \param[in] data Pointer to a WebData object
*/
PRIVATE void WebData_free(struct WebData *data) {

  if(data) {
    am_free(data->url);
    am_free(data->content_filename);
    //HTTPData_free(data->header);
    HTTPData_free(data->response);
    am_free(data);
    data = NULL;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/** \brief Create a new WebData object
*
* \param[in] url URL for a WebData object
*
* The parameter \a url is optional. You may provide \c NULL if no URL is required or not known yet.
*/
PRIVATE struct WebData* WebData_new(const char *url) {
  WebData *data = NULL;

  data = am_malloc(sizeof(WebData));
  if(!data)
    return NULL;

  data->url = NULL;
  data->content_filename = NULL;
  data->content_length = -1;
  //data->header = NULL;
  data->response = NULL;

  if(url) {
    data->url = am_strdup((char*)url);
  }

  //data->header = HTTPData_new();
  //if(!data->header) {
  //  WebData_free(data);
  //  return NULL;
  //}

  data->response = HTTPData_new();
  if(!data->response) {
    WebData_free(data);
    return NULL;
  }
  return data;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE void WebData_clear(struct WebData *data) {

  if(data) {
    am_free(data->content_filename);
    data->content_filename = NULL;
    //if(data->header) {
    //  am_free(data->header->data);
    //  data->header->data = NULL;
    //  data->header->size = 0;
    //}

    if(data->response) {
      am_free(data->response->data);
      data->response->data = NULL;
      data->response->size = 0;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE HTTPResponse* HTTPResponse_new(void) {
  HTTPResponse* resp = (HTTPResponse*)am_malloc(sizeof(struct HTTPResponse));
  if(resp) {
    resp->size = 0;
    resp->responseCode = 0;
    resp->data = NULL;
    resp->content_filename = NULL;
  }
  return resp;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

PUBLIC void HTTPResponse_free(struct HTTPResponse *response) {
  if(response) {
    am_free(response->data);
    am_free(response->content_filename);
    am_free(response);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE CURL* am_curl_init(const char* auth, uint8_t isPost) {
  CURL * curl = curl_easy_init();

  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L );
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data_callback);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_header_callback);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, getenv( "AM_CURL_VERBOSE" ) != NULL);
  curl_easy_setopt(curl, CURLOPT_POST, isPost ? 1 : 0);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L );
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 55L );
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
  curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 600L );
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L );
  //~ curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L );
  //curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L );
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L );
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L );

  if(auth && *auth) {
    dbg_printf(P_INFO2, "auth: %s", auth);
    curl_easy_setopt(curl, CURLOPT_USERPWD, auth);
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY );
  }

  dbg_printf(P_INFO2, "[am_curl_init] Created new curl session %p", (void*)curl);

  return curl;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/** \brief Download data from a given URL
*
* \param[in] url URL of the object to download
* \return a WebData object containing the complete response as well as header information
*
* getHTTPData() attempts to download the file pointed to by \a url and stores the content in a WebData object.
* The function returns \c NULL if the download failed.
*/

PUBLIC HTTPResponse* getHTTPData(const char *url, const char *cookies, CURL ** curl_session) {
  CURLcode      res;
  CURL         *curl_handle = NULL;
  CURL         *session = *curl_session;
  char         *escaped_url = NULL;
  WebData      *data = NULL;
  HTTPResponse *resp = NULL;
  int responseCode = -1;

  if(!url) {
    return NULL;
  }

  data = WebData_new(url);

  if(!data) {
    return NULL;
  }

  dbg_printf(P_INFO2, "[getHTTPData] url=%s, curl_session=%p", url, (void*)session);
  if(session == NULL) {
    if(gbGlobalInitDone == FALSE) {
      curl_global_init(CURL_GLOBAL_ALL);
      gbGlobalInitDone = TRUE;
    }
    session = am_curl_init(NULL, FALSE);
    *curl_session = session;
  }

  curl_handle = session;

  if(curl_handle) {
    escaped_url = url_encode_whitespace(url);
    assert(escaped_url);
    curl_easy_setopt(curl_handle, CURLOPT_URL, escaped_url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, data);

    if(cookies && *cookies) {
      /* if there's an explicit cookie string, use it */
      curl_easy_setopt(curl_handle, CURLOPT_COOKIE, cookies);
    } else {
      /* otherwise, enable cookie-handling since there might be cookies defined within the URL */
      curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, "");
    }

    res = curl_easy_perform(curl_handle);
    /* curl_easy_cleanup(curl_handle); */
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &responseCode);
    dbg_printf(P_INFO2, "[getHTTPData] response code: %d", responseCode);
    if(res != 0) {
        dbg_printf(P_ERROR, "[getHTTPData] '%s': %s (retval: %d)", url, curl_easy_strerror(res), res);
    } else {
      /* Only the very first connection attempt (where curl_session == NULL) should store the session,
      ** and only the last one should close the session.
      */
      resp = HTTPResponse_new();
      resp->responseCode = responseCode;
      //copy data if present
      if(data->response->data) {
        resp->size = data->response->size;
        resp->data = am_strndup(data->response->data, resp->size);
      }
      //copy filename if present
      if(data->content_filename) {
        resp->content_filename = am_strdup(data->content_filename);
      }
    }
    am_free(escaped_url);
  } else {
    dbg_printf(P_ERROR, "curl_handle is uninitialized!");
    resp = NULL;
  }
  WebData_free(data);
  return resp;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAXLEN 200

/** \brief Upload data to a specified URL.
*
* \param url Path to where data shall be uploaded
* \param auth (Optional) authentication information in the form of "user:password"
* \param data Data that shall be uploaded
* \param data_size size of the data
* \return Web server response
*/
PUBLIC HTTPResponse* sendHTTPData(const char *url, const char* auth, const void *data, uint32_t data_size) {
  CURL *curl_handle = NULL;
  CURLcode res;
  long rc, tries = 2, len;
  WebData* response_data = NULL;
  HTTPResponse* resp = NULL;
  char sessionKey[MAXLEN];

  struct curl_slist * headers = NULL;

  if( !url || !data ) {
    return NULL;
  }

  response_data = WebData_new(url);

  do {
    --tries;
    WebData_clear(response_data);

    if( curl_handle == NULL) {
      if(gbGlobalInitDone == FALSE) {
        curl_global_init(CURL_GLOBAL_ALL);
        gbGlobalInitDone = TRUE;
      }
      if( ( curl_handle = am_curl_init(auth, TRUE) ) ) {
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, response_data);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, response_data);
        //Transmission-specific options for HTTP POST
        if(strstr(response_data->url, "transmission") != NULL) {
          curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, parse_Transmission_response );
         headers = curl_slist_append(headers, "Content-Type: application/json");
          if( gSessionID ) {
            if((len = snprintf(sessionKey, MAXLEN, "X-Transmission-Session-Id: %s", gSessionID)) > 0) {
              sessionKey[len] = '\0';
            }
            headers = curl_slist_append(headers, sessionKey);
          }
          curl_easy_setopt( curl_handle, CURLOPT_HTTPHEADER, headers );
        }
        curl_easy_setopt(curl_handle, CURLOPT_URL, response_data->url);
      } else {
        dbg_printf(P_ERROR, "am_curl_init() failed");
        break;
      }
    }

    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data_size);

    if( ( res = curl_easy_perform(curl_handle) ) ) {
      dbg_printf(P_ERROR, "Upload to '%s' failed: %s", url, curl_easy_strerror(res));
      break;
    } else {
      curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &rc);
      dbg_printf(P_INFO2, "response code: %ld", rc);
      if(rc == 409) {
        if(gSessionID) {
          dbg_printf(P_INFO2, "Error code 409, session ID: %s", gSessionID);
        } else {
          dbg_printf(P_INFO2, "Error code 409, no session ID");
        }
        closeCURLSession( curl_handle );
        curl_slist_free_all( headers );
        headers = NULL;
        curl_handle = NULL;
      } else {
        resp = HTTPResponse_new();
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &resp->responseCode);

        //copy data if present
        if(response_data->response->data) {
          resp->size = response_data->response->size;
          resp->data = am_strndup(response_data->response->data, resp->size);
        }
        //copy filename if present
        if(response_data->content_filename) {
          resp->content_filename = am_strdup(response_data->content_filename);
        }
        break;
      }
    }
  } while(tries > 0);

  /* cleanup */
  closeCURLSession(curl_handle);

  if(headers) {
    curl_slist_free_all(headers);
  }

  WebData_free(response_data);

  return resp;
}

PUBLIC void closeCURLSession(CURL* curl_handle) {
  if(curl_handle) {
    dbg_printf(P_INFO2, "[closeCURLSession] Closing curl session %p", (void*)curl_handle);
    curl_easy_cleanup(curl_handle);
    curl_handle = NULL;
  }
}
