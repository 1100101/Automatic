#define MSGSIZE_MAX 700
#include "list.h"
enum debug_type {
	P_ERROR,
	P_MSG,
	P_INFO,
	P_INFO2
};

typedef enum debug_type debug_type;

void dbg_printf(debug_type type, const char *format, ...);
void print_list(linked_list list);
