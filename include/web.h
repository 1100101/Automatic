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

#define MAX_URL_LEN 1024

/** Generic struct storing data and the size of the contained data */
struct HTTPData {
	/** \{ */
 char *data; 	/**< Stored data */
 size_t size; /**< Size of the stored data */
 /** \{ */
};

/** Struct storing information about data downloaded from the web */
struct WebData {
	/** \{ */
	char *url;                  /**< URL of the WebData object */
	long responseCode;          /**< HTTP response code        */
	size_t content_length;      /**< size of the received data determined through header field "Content-Length" */
	char *content_filename;     /**< name of the downloaded file determined through header field "Content-Length" */
	struct HTTPData* header;    /**< complete header information in a HTTPData object */
	struct HTTPData* response;  /**< HTTP response in a HTTPData object */
	/** \} */
};

typedef struct HTTPData HTTPData;
typedef struct WebData WebData;

WebData* getHTTPData(const char *url);
char* 	 sendHTTPData(const char *url, const char* auth, const void *data, unsigned int data_size);
void 		 WebData_free(struct WebData *data);


#endif /* WEB_H_ */
