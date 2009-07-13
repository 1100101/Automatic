/*
 * prowl.h
 *
 *  Created on: Jul 13, 2009
 *      Author: Frank Aurich
 */

#ifndef PROWL_H__
#define PROWL_H__

#include <inttypes.h>


int8_t sendProwlNotification(const char* apikey, const char* event, const char* desc);
int8_t verifyProwlAPIKey(const char* apikey);


#endif //PROWL_H__
