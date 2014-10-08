/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file state.c
 *
 * Load and save the download history for Automatic.
 */

/*
 * Copyright (C) 2008 Frank Aurich (1100101+automatic@gmail.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/param.h>

#include "output.h"
#include "utils.h"
#include "list.h"

#ifdef MEMWATCH
   #include "memwatch.h"
#endif

/** \cond */
#define MAX_LINE_LEN 2048
/** \endcond */

/** \brief Store the URLs of the downloaded torrents on disk for later retrieval
 *
 * \param state_file Name of the file to save to
 * \param downloads (bucket)list containing the URLs of all downloaded torrents
 *
 * save_state() stores the content of the torrent bucket list on disk so Automatic won't
 * download old torrents after a restart.
 */
int save_state(const char* state_file, const simple_list const downloads) {
   FILE *fp;
   NODE *current = NULL;
   int result;

   if(state_file) {
      current = downloads;
      dbg_printf(P_MSG, "Saving state (%d downloaded torrents) to disk", listCount(current));
      if((fp = fopen(state_file, "w")) == NULL) {
         dbg_printf(P_ERROR, "Error: Unable to open statefile '%s' for writing: %s", state_file, strerror(errno));
         return -1;
      }

      while (current != NULL && current->data != NULL) {
         result = fputs((const char*)current->data, fp);
         if(result != EOF) {
            result = fputc('\n', fp);
         }

         if(result == EOF)
         {
            dbg_printf(P_ERROR, "Error: Unable to write to statefile '%s': %s", state_file, strerror(errno));
            fclose(fp);
            return -1;
         }

         current = current->next;
      }

      fclose(fp);
      dbg_printf(P_INFO, "Done saving state");
   }
   return 0;
}

/** \brief Load an old state from disk.
 *
 * \param state_file Path to the state file
 * \param head Pointer to a (bucket-)list
 *
 * load_state() reads the URLs from state_file and stores them in a bucket list.
 * This way Automatic won't download old torrents again after, e.g. a restart.
 */
int load_state(const char* state_file, NODE **head) {
   FILE *fp;
   int len;
   char line[MAX_LINE_LEN];
   char *data;

   if((fp = fopen(state_file, "r")) == NULL) {
      dbg_printf(P_ERROR, "[load_state] Error: Unable to open statefile '%s' for reading: %s", state_file, strerror(errno));
      return -1;
   }

   while (fgets(line, MAX_LINE_LEN, fp)) {
      len = strlen(line);
      data = am_strndup(line, len-1);  /* len-1 to get rid of the \n at the end of each line */
      addToTail(data, head);
   }

   fclose(fp);
   dbg_printf(P_MSG, "Restored %d old entries", listCount(*head));
   return 0;
}

