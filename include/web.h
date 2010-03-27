#ifndef WEB_H__
#define WEB_H__

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

#include <unistd.h>
#include <curl/curl.h>

#define MAX_URL_LEN 1024

#ifndef FALSE
  #define FALSE 0
#endif

#ifndef TRUE
  #define TRUE 1
#endif    


struct HTTPResponse {
 char    *data;
 char    *content_filename; /**< name of the downloaded file determined through header field "Content-Length" */
 size_t   size;             /**< size of the received data determined through header field  "Content-Length" */
 uint16_t responseCode;
};

typedef struct HTTPResponse HTTPResponse;

HTTPResponse* getHTTPData(const char *url, const char *cookies, CURL **curl_handle);
HTTPResponse* sendHTTPData(const char *url, const char* auth, const void *data, unsigned int data_size);
void     HTTPResponse_free(struct HTTPResponse *response);
void     SessionID_free(void);
void     closeCURLSession(CURL* curl_handle);

#endif /* WEB_H_ */
