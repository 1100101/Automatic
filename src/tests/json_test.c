/*
 * base64_test.c
 *
 *  Created on: Oct 20, 2008
 *      Author: aurich
 */


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "output.h"
#include "json.h"


int8_t verbose = P_NONE;

int main(void) {

	const char *str = "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZy4=";

  uint32_t size;
  char* json = makeJSON(NULL, 0, 0, &size);
  assert(json == NULL && size == 0);
  json = makeJSON(NULL, 20, 0, &size);
  assert(json == NULL && size == 0);
  json = makeJSON(str, 0, 0, &size);
  assert(json == NULL && size == 0);
  json = makeJSON(str, strlen(str), 1, &size);
  assert(strlen(json) == size && size == 152);
  am_free(json);
  json = makeJSON(str, strlen(str), 0, &size);
  assert(strlen(json) == size && size == 152);
  am_free(json);
  return 0;
}
