/*
 * prowl.h
 *
 *  Created on: Jul 13, 2009
 *      Author: Frank Aurich
 */

#ifndef PROWL_H__
#define PROWL_H__

#include <inttypes.h>

enum prowl_event {
  PROWL_NEW_DOWNLOAD = 1,
  PROWL_DOWNLOAD_FAILED = 2
};

typedef enum prowl_event prowl_event;


int8_t prowl_sendNotification(enum prowl_event event, const char* apikey, const char *filename);

int16_t sendProwlNotification(const char* apikey, const char* event, const char* desc);
int16_t verifyProwlAPIKey(const char* apikey);


#endif //PROWL_H__
