/*
 * transmission.h
 *
 *  Created on: Oct 13, 2008
 *      Author: aurich
 */

#ifndef TRANSMISSION_H_
#define TRANSMISSION_H_

int8_t call_transmission(const char* tm_path, const char *filename);
int8_t call_external(const char *external_name, const char *filename);
int8_t getRPCVersion(const char* host, uint16_t port, const char* auth);

#endif /* TRANSMISSION_H_ */
