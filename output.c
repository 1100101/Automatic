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
		if(nofork == 0 && strlen(log_file) > 1) {
			fp = fopen(log_file,"a");
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


void print_list(linked_list list) {
	NODE *cur = list;
	while(cur != NULL) {
		if(cur->item.name != NULL) {
			dbg_printf(P_INFO2, "  name:\t%s\n", cur->item.name);
		}
		if(cur->item.url != NULL) {
			dbg_printf(P_INFO2, "  URL:\t%s\n", cur->item.url);
		}
		cur = cur->pNext;
	}
}
