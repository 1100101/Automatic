/*
 * json.h
 *
 *  Created on: Oct 13, 2008
 *      Author: aurich
 */

#ifndef JSON_H_
#define JSON_H_

char* makeJSON(const void *data, uint32_t tsize, uint8_t start, uint32_t *setme_size);
char* makeChangeUpSpeedJSON(uint8_t tID, uint32_t upspeed, uint8_t rpcVersion, uint32_t *setme_size);
char* parseResponse(const char* response);
int8_t parseTorrentID(const char* response);
int8_t parseRPCVersion(const char* response);

#endif /* JSON_H_ */
