/*
 * file.c
 *
 *  Created on: Oct 12, 2008
 *      Author: kylek
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utils.h"
#include "output.h"


char* readFile(const char *fname, uint32_t * setme_len) {
  FILE *file;
  char *buffer = NULL;
  uint32_t fileLen;

  file = fopen(fname, "rb");
  if (!file) {
    dbg_printf(P_ERROR, "Cannot open file '%s': %s", fname, strerror(errno));
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  fileLen = ftell(file);
  fseek(file, 0, SEEK_SET);

  buffer = am_malloc(fileLen + 1);
  if (!buffer) {
    dbg_printf(P_ERROR, "Cannot allocate memory: %s", strerror(errno));
    fclose(file);
    return NULL;
  }

  fread(buffer, fileLen, 1, file);
  buffer[fileLen] = '\0';
  fclose(file);
  if (setme_len) {
    *setme_len = fileLen;
  }
  return buffer;
}

int saveFile(const char *name, void *data, uint32_t size) {
  int torrent, ret = -1;

  if (!name || !data) {
    return -1;
  }

  torrent = open(name, O_RDWR | O_CREAT, 00644);
  if (torrent == -1) {
    dbg_printf(P_ERROR, "Error opening file for writing: %s", strerror(errno));
    ret = -1;
  } else {
    ret = write(torrent, data, size);
    if (ret != size) {
      dbg_printf(P_ERROR, "[saveFile] Error writing torrent file: %s",
          strerror(errno));
      ret = -1;
    } else {
      dbg_printf(P_INFO, "Saved torrent file '%s'", name);
    }
    fchmod(torrent, 0644);
    close(torrent);
    ret = 0;
  }
  return ret;
}
