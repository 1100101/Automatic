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
#include <sys/param.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include "output.h"
#include "automatic.h"

void dbg_printf(debug_type type, const char *format, ...) {
	va_list va;
   char tmp[MSGSIZE_MAX];
	int print_msg = 0;
	FILE *fp;
	const char *log_file = NULL;

	uint8_t nofork  = am_get_nofork();
	uint8_t verbose = am_get_verbose();

	if(nofork == 0)
		log_file = am_getlogfile();

	switch(type) {
		case P_ERROR: /* fallthrough */
		case P_MSG: {
			print_msg = 1;
			break;
		}
		case P_INFO: {
			if(verbose > 1)
				print_msg = 1;
			break;
		}

		default: {
			if(verbose > 2)
				print_msg = 1;
			break;
		}
	}
	if(print_msg) {
		va_start(va, format);
		vsnprintf(tmp, MSGSIZE_MAX, format, va);
		va_end(va);
		tmp[MSGSIZE_MAX-1] = '\0';
		if(nofork == 0 && log_file != NULL && strlen(log_file) > 1) {
			fp = fopen(log_file,"a");
			if(fp) {
				fprintf(fp,"%s\n", tmp);
				fflush(fp);
				fclose(fp);
			}
		} else {
			fprintf(stderr, "%s\n", tmp);
			fflush(stderr);
		}

	}
}

void getlogtime_str(char *buf) {
	char tmp[TIME_STR_SIZE];
	time_t now;
	struct tm now_tm;
	struct timeval tv;

	now = time( NULL );
	gettimeofday( &tv, NULL );

	localtime_r( &now, &now_tm );
	strftime( tmp, sizeof(tmp), "%y/%m/%d %H:%M:%S", &now_tm );
	snprintf( buf, strlen(tmp) + 1, "%s", tmp);
}
