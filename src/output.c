/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file output.c
 *
 * Provides output functionality.
 */

/* Copyright (C) 2008 Frank Aurich (1100101+automatic@gmail.com
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
#include <sys/param.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include "output.h"

#define MSGSIZE_MAX     5000

static FILE   *gLogFP = NULL;
static int8_t  gMsglevel;

static const char *gPidfile = NULL;

unsigned char log_init(const char *logfile, char msglevel, char append_log) {
  gMsglevel = msglevel;
  if(logfile && *logfile) {
    /* Just in case someone decides to call this function twice, make sure the previous log is closed. */
    if(gLogFP) {
      log_close();
    }
    if((gLogFP = fopen(logfile, append_log ? "a" : "w")) == NULL) {
      /* this should work just fine: the message level has been set above, and if gLogFP is NULL,
      ** logging goes to stderr.
      */
      dbg_printf(P_ERROR, "[log_init] Opening '%s' for logging failed", logfile);
      return 1; //bad (well, not really)
    }
  }
  return 1; //all good
}

void log_close(void) {
  if(gLogFP) {
    fclose(gLogFP);
    gLogFP = NULL;
  }
}

unsigned char pid_create(const char *pidfile, int pid) {
  FILE   *gPidFP;

  if(pidfile && *pidfile) {
    if((gPidFP = fopen(pidfile, "w")) == NULL) {
      dbg_printf(P_ERROR, "[pid_create] Opening '%s' for pidfile failed", pidfile);
      return 1; //bad (well, not really)
    } else {
      dbg_printf(P_MSG, "[pid_create] Opening '%s' for pidfile (pid:%d)", pidfile, pid);
      fprintf(gPidFP, "%d\n", pid);
      fclose(gPidFP);
      gPidfile = pidfile;
    }
  }
  return 1; //all good
}

void pid_close() {
  if(gPidfile && *gPidfile) {
    remove(gPidfile);
    gPidfile = NULL;
  }
}


/** \brief Print log information to stdout.
 *
 * \param[in] type Type of logging statement.
 * \param[in] format Format string
 * \param[in] ... Additional parameters
 *
 * dbg_printf() prints logging and debug information to stdout. The relevance of each
 * statement is defined by the given type. The end-user provides a verbosity level (e.g.
 * on the command-line) which dictates what kind of messages are printed and which not.
 */
void am_printf( const char * file, int line, debug_type type, int withTime, const char * format, ... ) {
  va_list va;
  char tmp[MSGSIZE_MAX];
  char timeStr[TIME_STR_SIZE];
  FILE *fp = NULL;

  if(gMsglevel >= type) {
    va_start(va, format);
    vsnprintf(tmp, MSGSIZE_MAX, format, va);
    va_end(va);
    tmp[MSGSIZE_MAX-1] = '\0';

    fp = gLogFP ? gLogFP : stderr; /* log to stderr in case no logfile has been specified */
    if(withTime) {
      fprintf(fp, "[%s] ", getlogtime_str(timeStr));
    }

    if(P_INFO2 >= type || P_ERROR == type) {
      fprintf(fp, "%s, %d: %s\n", file, line, tmp);
    } else {
      fprintf(fp,"%s\n", tmp);
    }
    fflush(fp);
  }
}

/** \brief Get the current date and time in form of a string.
 *
 * \param buf Pointer to a string buffer which will hold the time string.
 * \return The current time as a string.
 *
 * This function is useful for logging purposes.
 */
char* getlogtime_str(char *buf) {
	char tmp[TIME_STR_SIZE];
	time_t now;
	struct tm now_tm;
	struct timeval tv;

	now = time( NULL );
	gettimeofday( &tv, NULL );

	localtime_r( &now, &now_tm );
	strftime( tmp, sizeof(tmp), "%y/%m/%d %H:%M:%S", &now_tm );
	snprintf( buf, strlen(tmp) + 1, "%s", tmp);
	return buf;
}
