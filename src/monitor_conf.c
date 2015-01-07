/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file monitor_conf.c
 *
 * Provides output functionality.
 */

/* Copyright (C) 2015 Benoit Masson (yahoo@perenite.com)
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <fcntl.h>

#include "utils.h"
#include "output.h"
#include "monitor_conf.h"

#define MAX_EVENTS 1024 /*Max. number of events to process at one go*/
#define LEN_NAME 16 /*Assuming that the length of the filename won't exceed 16 bytes*/
#define EVENT_SIZE  ( sizeof (struct inotify_event) ) /*size of one event*/
#define BUF_LEN     ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )) /*buffer to store the data of events*/

PRIVATE int fd = 0;
PRIVATE int wd = 0;
PRIVATE char *buffer;

unsigned int monitorConf_init(const char *config_file) {

  /* check fd, wd are null if not close them */
  if (fd || wd ) {
    monitorConf_close();
  }

  dbg_printf(P_DBG, "Monitor Conf init");

  fd = inotify_init();
  if ( fd < 0 ) {
    dbg_printf(P_ERROR, "Couldn't initialize inotify");
    return 1;
  }

  /* non blocking inotify */
  if (fcntl (fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) <0 ) {
    dbg_printf(P_ERROR, "Couldn't initialize non blocking IO inotify");
    monitorConf_close();
    return 1;
  }

  /* add watch to config directory */
  wd = inotify_add_watch(fd, config_file, IN_CREATE | IN_MODIFY | IN_DELETE);

  if (wd == -1) {
    dbg_printf(P_ERROR, "Couldn't add watch to %s\n",config_file);
  } else {
    dbg_printf(P_MSG, "Watching: %s\n",config_file);
  }

  /* inititialize buffer for inotify read events */
  buffer = (char *) malloc(BUF_LEN);

  return 0;
}

void monitorConf_close(void) {
  dbg_printf(P_DBG, "Monitor Conf close");
  if (fd || wd) {
    inotify_rm_watch(fd, wd);
  }
  free(buffer);
}

bool monitorConf_hasChanged(void) {
  bool changed = false;
  struct inotify_event *event = NULL;
  int length = 0;
  int i = 0;

  dbg_printf(P_DBG, "Monitor file conf check haschanged");

  if ( fd ) {
    dbg_printf(P_DBG, "Monitor file conf fd > 0");

    dbg_printf(P_DBG, "Monitor file conf before read");
    length = read( fd, buffer, BUF_LEN );
    dbg_printf(P_DBG, "Monitor file conf after read, length:%d", length);

    if (length >0) { /* >0 include both no data & EAGAIN error for non-blocking */
      while ( i < length ) {
        event = ( struct inotify_event * ) &buffer[ i ];
        dbg_printf(P_DBG, "Monitor file conf event->len:%d , event->mask:%d", event->len, event->mask);
        if ( event->mask & IN_MODIFY) {
          dbg_printf(P_INFO2, "Monitor file conf MODIFY event detected");
          changed = true;
        }
        i += EVENT_SIZE + event->len;
      }
    }
  }

  dbg_printf(P_DBG, "Monitor file conf check haschanged end"); 
  return changed;
}
