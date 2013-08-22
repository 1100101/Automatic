/*
 * toasty.h
 *
 *  Created on: August 22, 2013
 *      Author: Frank Aurich
 */

#ifndef TOASTY_H__
#define TOASTY_H__

#include <inttypes.h>
#include "prowl.h"

int8_t toasty_sendNotification(enum prowl_event event, const char* deviceId, const char *filename);

int16_t sendToastyNotification(const char* deviceId, const char* event, const char* desc);

#endif //TOASTY_H__
