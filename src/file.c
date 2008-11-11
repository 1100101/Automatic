/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file file.c
 *
 * Read and write files
 *
 * \internal Created on: Oct 12, 2008
 * \internal Author: Frank Aurich (1100101+automatic@gmail.com)
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

/** \brief Read a file and store its content in memory.
 *
 * \param[in] fname (input) filename
 * \param[out] setme_len (output) number of read bytes
 * \return pointer to file content
 *
 * The function returns NULL if the specified file could not be properly read.
 */
char* readFile(const char *fname, uint32_t *setme_len) {
  FILE *file;
  char *buffer = NULL;
  uint32_t fileLen;

/*  if(!fname || !setme_len) {
  	return NULL;
  }
*/
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

/** \brief Write data to a file.
 *
 * \param[in] name (input) filename
 * \param[in] data (input) pointer to data
 * \param[in] size (input) size of data
 * \return 0 if saving was successful, -1 otherwise.
 */
int saveFile(const char *name, const void *data, uint32_t size) {
  int fh, ret = -1;

  if (!name || !data) {
    return -1;
  }

  fh = open(name, O_RDWR | O_CREAT, 00644);
  if (fh == -1) {
    dbg_printf(P_ERROR, "Error opening file for writing: %s", strerror(errno));
    ret = -1;
  } else {
    ret = write(fh, data, size);
    if ((uint32_t)ret != size) {
      dbg_printf(P_ERROR, "[saveFile] Error writing torrent file: %s",
          strerror(errno));
      ret = -1;
    } else {
      dbg_printf(P_INFO, "Saved torrent file '%s'", name);
    }
    fchmod(fh, 0644);
    close(fh);
    ret = 0;
  }
  return ret;
}
