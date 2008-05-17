#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <stdarg.h>
#include "output.h"

extern char logfile[MAXPATHLEN];
extern int verbose;
extern int gl_debug;

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
		if(gl_debug == 0) {
			fp = fopen(logfile,"a");
			if(fp) {
				fprintf(fp,"%s", tmp);
				fflush(fp);
				fclose(fp);
			}
		} else {
			fprintf(stderr, "%s", tmp);
			fflush(stderr);
		}

	}
}
