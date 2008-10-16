/*
 * file.h
 *
 *  Created on: Oct 12, 2008
 *      Author: kylek
 */

#ifndef FILE_H_
#define FILE_H_

#include <stdint.h>

char* readFile(const char *fname, uint32_t * setme_len);
int saveFile(const char *name, const void *data, uint32_t size);

#endif /* FILE_H_ */
