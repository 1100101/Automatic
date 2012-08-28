/*
 * base64_test.c
 *
 *  Created on: Oct 20, 2008
 *      Author: aurich
 */


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "output.h"
#include "json.h"

int8_t verbose = P_NONE;

int main(void) {

	const char *str = "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZy4=";

  uint32_t size;
  dbg_printf(P_INFO, "makeTorrentAddMetaInfoJSON(NULL, 0 , ...)");
  char* json = makeTorrentAddMetaInfoJSON(NULL, 0, 0, NULL, &size);
  assert(json == NULL && size == 0);
  dbg_printf(P_INFO, "makeTorrentAddMetaInfoJSON(NULL,  20, ...)");
  
  json = makeTorrentAddMetaInfoJSON(NULL, 20, 0, NULL, &size);
  assert(json == NULL && size == 0);
  dbg_printf(P_INFO, "makeTorrentAddMetaInfoJSON(str, 0, ...)");
  
  json = makeTorrentAddMetaInfoJSON(str, 0, 0, NULL, &size);
  assert(json == NULL && size == 0);
  dbg_printf(P_INFO, "makeTorrentAddMetaInfoJSON(str, strlen(str), 1, ...)");
  
  json = makeTorrentAddMetaInfoJSON(str, strlen(str), 1, NULL, &size);
  dbg_printf(P_INFO, "\n%s\n (%d)", json, size);
  assert(strlen(json) == size && size == 153);
  am_free(json);
  
  dbg_printf(P_INFO, "makeTorrentAddMetaInfoJSON(str, strlen(str), 0, ...)");
  json = makeTorrentAddMetaInfoJSON(str, strlen(str), 0, NULL, &size);
  assert(strlen(json) == size && size == 153);
  am_free(json);
  return 0;
}
