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
#include "output.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

extern char log_file[MAXPATHLEN + 1];
extern uint8_t verbose;
extern uint8_t nofork;
extern uint8_t daemonized;

void dbg_printf(debug_type type, const char *format, ...) {
	va_list va;
   char tmp[MSGSIZE_MAX];
	int print_msg = 0;
	FILE *fp;
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
#ifdef DEBUG
			if(verbose > 2)
				print_msg = 1;
#endif
			break;
		}
	}
	if(print_msg) {
		va_start(va, format);
		vsnprintf(tmp, MSGSIZE_MAX, format, va);
		va_end(va);
		tmp[MSGSIZE_MAX-1] = '\0';
		if(daemonized == 1 && nofork == 0 && strlen(log_file) > 1) {
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
