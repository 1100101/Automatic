#include <assert.h>
#include <string.h>

#include "utils.h"
#include "output.h"
#include "config_parser.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

#define MAX_OPT_LEN	50
#define MAX_PARAM_LEN	20000

static const char *delim = "²";

static char* l_shorten(const char *str) {

    int tmp_pos;
    char c;
    char *retStr;
    char *tmp = (char*)am_malloc(MAX_PARAM_LEN+1);
    uint32_t line_pos = 0, i;
    uint32_t len = strlen(str);

    if(!tmp) {
      dbg_printf(P_ERROR, "[shorten] calloc(MAX_PARAM_LEN) failed!");
      return NULL;
    }

    memset(tmp, 0, MAX_PARAM_LEN+1);

    while (isspace(str[line_pos])) {
      ++line_pos;
    }
    tmp_pos = 0;
    while(line_pos < len) {
      /* case 1: quoted strings */
      if(tmp_pos != 0) {
        for(i = 0; i < strlen(delim); ++i)
          tmp[tmp_pos++] = delim[i];
      }
      if (str[line_pos] == '"' || str[line_pos] == '\'') {
        c = str[line_pos];
        ++line_pos;  /* skip quote */
        while(str[line_pos] != c && line_pos < len && str[line_pos] != '\n' && str[line_pos] != '\0') {
          tmp[tmp_pos++] = str[line_pos++];
        }
        if(str[line_pos] == c) {
          line_pos++; /* skip the closing quote */
        }
      } else {
        for(; isprint(str[line_pos]) && !isspace(str[line_pos]); /* NOTHING */) {
          tmp[tmp_pos++] = str[line_pos++];
        }
      }
      while (isspace(str[line_pos])) {
        ++line_pos;
      }
    }
    tmp[tmp_pos] = '\0';
    assert(strlen(tmp) < MAX_PARAM_LEN);
    retStr = am_strdup(tmp);
    am_free(tmp);
    return retStr;
}

int main(void) {
  const char *str = "        \"test1\"\n"
                    "        \"test2\"\n"
                    "        \"test3239823982\"\n"
                    "        \"test4489278sdjhsd8923";

  char *x = l_shorten(str);

	return 0;
}
