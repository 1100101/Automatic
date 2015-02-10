/*
 * pushalot.h
 *
 *  Created on: Feb 19, 2014
 *      Author: Frank Aurich
 */

#ifndef PUSHALOT_H__
#define PUSHALOT_H__

#include <inttypes.h>

enum pushalot_event {
  PUSHALOT_NEW_DOWNLOAD = 1,
  PUSHALOT_DOWNLOAD_FAILED = 2
};

typedef enum pushalot_event pushalot_event;


int8_t pushalot_sendNotification(enum pushalot_event event, const char* apikey, const char *filename);

int16_t sendPushalotNotification(const char* apikey, const char* event, const char* desc);


#endif //PUSHALOT_H__
