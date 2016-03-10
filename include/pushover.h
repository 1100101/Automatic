/*
 * pushover.h
 *
 * Created on: Oct 5, 2014
 * Author: sew
 */

#ifndef PUSHOVER_H__
#define PUSHOVER_H__

#include <inttypes.h>

enum pushover_event {
  PUSHOVER_NEW_DOWNLOAD = 1,
  PUSHOVER_DOWNLOAD_FAILED = 2
};

typedef enum pushover_event pushover_event;


int8_t pushover_sendNotification(enum pushover_event event, const char* apikey, const char *filename);

int16_t sendPushoverNotification(const char* apikey, const char* event, const char* desc);


#endif //PUSHOVER_H__
