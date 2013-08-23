/* $Id$
* $Name$
* $ProjectName$
*/

/**
* @file config_parser.c
*
* Parse configuration file.
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
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "automatic.h"
#include "config_parser.h"
#include "filters.h"
#include "list.h"
#include "output.h"
#include "regex.h"
#include "rss_feed.h"
#include "utils.h"


/** \cond */
#define MAX_OPT_LEN  50
#define MAX_PARAM_LEN  20000

struct suboption {
  char *option;
  char *value;
};

typedef struct suboption suboption_t;
/** \endcond */

PRIVATE void freeOptionItem(void* item) {
  if(item != NULL) {
    suboption_t* obj = (suboption_t*)item;
    am_free(obj->option);
    am_free(obj->value);
    am_free(obj);
  }
}

PRIVATE void set_path(const char *src, char **dst) {
  char *tmp;

  if(src && strlen(src) < MAXPATHLEN) {
    tmp = resolve_path(src);
    if(tmp) {
      if ( *dst != NULL ) {
        am_free(*dst);
      }

      *dst = am_replace_str(tmp, "\\ ", " ");

      if(*dst == NULL) {
        dbg_printf(P_ERROR, "[set_path] Error executing am_replace_str()!");
      }

      am_free(tmp);
    }
  }
}

/* http://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way */
PRIVATE char* trim(const char *str) {
  const char *end;

  if(!str || !*str) {
    return NULL;
  }

  // Trim leading space
  while(isspace(*str)) {
    str++;
  }

  if(*str == 0)  // All spaces?
  {
    return NULL;
  }

  /* skip single or double quote */
  if (*str == '"' || *str == '\'') {
    str++;
  }

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) {
    end--;
  }

  /* skip single or double quote */
  if (*end == '"' || *end == '\'') {
    end--;
  }

  end++;

  // Return copy of trimmed string
  return am_strndup(str, end - str);
}


PRIVATE int parseUInt(const char *str) {
  int is_num = 1;
  uint32_t i;
  uint32_t result = -1;

  char * trimmed = trim(str);

  for(i = 0; i < strlen(trimmed); i++) {
    if(isdigit(trimmed[i]) == 0) {
      is_num--;
    }
  }

  if(is_num == 1 && atoi(trimmed) > 0) {
    result = atoi(trimmed);
  }

  am_free(trimmed);
  return result;
}

PRIVATE suboption_t* parseSubOption(char* line) {
  const char *subopt_delim = "=>";
  uint32_t i = 0;

  suboption_t* option_item = NULL;
  char *option = NULL;
  char *param  = NULL;

  assert(line && *line);

  while(line[i] != '\0') {
    if(line[i] == subopt_delim[0] && line[i+1] == subopt_delim[1]) {
      option = am_strndup(line, i-1);
      param  = trim(line + i + strlen(subopt_delim));
      break;
    }

    i++;
  }

  if(option && param) {
    option_item = (suboption_t*)am_malloc(sizeof(suboption_t));
    option_item->option = option;
    option_item->value = param;
  }

  return option_item;
}

PRIVATE simple_list parseMultiOption(const char *str) {
  int tmp_pos;
  uint32_t line_pos = 0;
  uint32_t len = strlen(str);
  simple_list options = NULL;
  char tmp[MAX_PARAM_LEN];
  int last_dbl_quote_pos;
  int8_t parse_error = 0;
  int32_t current_line_pos = -1;

  if(len == 0) {
    dbg_printf(P_ERROR, "[parseMultiOption] empty input string!");
    return NULL;
  }

   while(line_pos < len) {
    memset(&tmp, 0, sizeof(tmp));
    // Skip any initial whitespace
    while (line_pos < len && isspace(str[line_pos])) {
      ++line_pos;
    }

    tmp_pos = 0;
    parse_error = 0;
    last_dbl_quote_pos = -1;

    while(line_pos < len && str[line_pos] != '\0') {
      if(str[line_pos] == '\"') {
        last_dbl_quote_pos = tmp_pos;
      } else if(str[line_pos] == '\n') {
        // Text is broken over multiple lines
        if(str[line_pos - 1] == '\\' || str[line_pos - 1] == '+') {
          // skip newline
          line_pos++;
          // skip whitespace at the beginning of the next line
          while (line_pos < len && isspace(str[line_pos])) {
            ++line_pos;
          }

          if(str[line_pos] == '\"' && last_dbl_quote_pos != -1) {
            // Reset the string index to the position of the last double-quote, and properly null-terminate it
            tmp_pos = last_dbl_quote_pos;
            tmp[tmp_pos] = '\0';

            // Skip the double-quote on the new line as well
            line_pos++;
          } else {
            tmp[tmp_pos] = '\0';
            dbg_printf(P_ERROR, "[parseMultiOption] Parsing error at line '%s'", &tmp[current_line_pos]);
            parse_error = 1;
            break;
          }
        } else {
          // If the character before the newline is not a backslash ('\'), consider this suboption complete
          break;
        }

        current_line_pos = tmp_pos;
      }

      tmp[tmp_pos++] = str[line_pos++];
    }

    if(parse_error) {
      break;
    }

    /* A suboption is finished, end it with a null terminator */
    tmp[tmp_pos] = '\0';

    /* store the line in our list */
    if(tmp_pos != 0) {
      suboption_t* i = parseSubOption(tmp);

      if(i != NULL) {
        addItem(i, &options);
      } else {
        dbg_printf(P_ERROR, "Invalid suboption string: '%s'", tmp);
      }
    }
  }

  return options;
}

PRIVATE int parseFilter(am_filters *filters, const char* filter_str) {
  am_filter filter = NULL;
  int32_t result = SUCCESS; /* be optimistic */
  simple_list option_list = NULL;
  NODE * current = NULL;
  suboption_t *opt_item = NULL;
  char *tmpStr = NULL;

  option_list = parseMultiOption(filter_str);
  current = option_list;

  while (current != NULL) {
    opt_item = (suboption_t*)current->data;
    if(opt_item != NULL) {
      if(!filter) {
        filter = filter_new();
        assert(filter && "filter_new() failed!");
      }

      if(!strncmp(opt_item->option, "pattern", 7)) {
        filter->pattern = trim(opt_item->value);
      } else if(!strncmp(opt_item->option, "folder", 6)) {
        tmpStr = trim(opt_item->value);
        set_path(tmpStr, &filter->folder);
        am_free(tmpStr);
      } else if(!strncmp(opt_item->option, "feedid", 6)) {
        filter->feedID = trim(opt_item->value);
      } else {
        dbg_printf(P_ERROR, "Unknown suboption '%s'!", opt_item->option);
      }
    } else {
      assert(0 && "opt_item == NULL");
    }

    current = current->next;
  }

  if(filter && filter->pattern) {
    filter_add(filter, filters);
  } else {
    dbg_printf(P_ERROR, "Invalid filter: '%s'", filter_str);
    result = FAILURE;
  }

  if(option_list != NULL) {
    freeList(&option_list, freeOptionItem);
  }

  return result;
}

PRIVATE void parseCookiesFromURL(rss_feed* feed) {
  const char* result_regex = ":COOKIE:(.+)";

  assert(feed && feed->url && *feed->url);

  feed->cookies = getRegExMatch(result_regex, feed->url, 1);
}

PRIVATE int parseFeed(rss_feeds *feeds, const char* feedstr) {
  rss_feed* feed = NULL;
  int32_t result = SUCCESS; /* be optimistic */
  simple_list option_list = NULL;
  NODE * current = NULL;
  suboption_t *opt_item = NULL;

  option_list = parseMultiOption(feedstr);
  current = option_list;

  while (current != NULL) {
    opt_item = (suboption_t*)current->data;

    if(opt_item != NULL) {
      if(!feed) {
        feed = feed_new();
        assert(feed && "feed_new() failed!");
      }

      if(!strncmp(opt_item->option, "url_pattern", 11)) {
        feed->url_pattern = trim(opt_item->value);
      } else if(!strncmp(opt_item->option, "url_replace", 11)) {
        feed->url_replace = trim(opt_item->value);
      } else if(!strncmp(opt_item->option, "url", 3)) {
        feed->url = trim(opt_item->value);
      } else if(!strncmp(opt_item->option, "cookies", 6)) {
        feed->cookies = trim(opt_item->value);
      } else if(!strncmp(opt_item->option, "id", 2)) {
        feed->id = trim(opt_item->value);
      } else {
        dbg_printf(P_ERROR, "Unknown suboption '%s'!", opt_item->option);
      }
    } else {
      assert(0 && "opt_item == NULL");
    }

    current = current->next;
  }

  if(feed && feed->url) {
    /* Maybe the cookies are encoded within the URL */
    if(feed->cookies == NULL) {
      parseCookiesFromURL(feed);
    }

    feed_add(feed, feeds);
  } else {
    dbg_printf(P_ERROR, "Invalid feed: '%s'", feedstr);
    result = FAILURE;
  }

  if(option_list != NULL) {
    freeList(&option_list, freeOptionItem);
  }

  return result;
}

/** \brief parse option from configuration file.
*
* \param[in,out] as Pointer to session handle
* \param[in] opt name of option to set (left of =)
* \param[in] param name of value for option (right of =)
* \param type type for param, currently unused
* \return 0 if parsing was successful, -1 if an error occured.  currently
* always returns 0
*/
PRIVATE int set_option(auto_handle *as, const char *opt, const char *param, option_type type) {
  int32_t numval;
  int32_t result = SUCCESS;

  dbg_printf(P_INFO2, "[config] %s=%s (type: %d)", opt, param, type);

  assert(as != NULL);
  if(!strcmp(opt, "url")) {
    dbg_printf(P_ERROR, "the 'url' option is not supported any more, please use the 'feed' option instead!");
    result = FAILURE;
  } else if(!strcmp(opt, "feed")) {
    result = parseFeed(&as->feeds, param);
  } else if(!strcmp(opt, "transmission-home")) {
    set_path(param, &as->transmission_path);
  } else if(!strcmp(opt, "prowl-apikey")) {
    as->prowl_key = am_strdup(param);
  } else if(!strcmp(opt, "toasty-deviceid")) {
    as->toasty_key = am_strdup(param);
  } else if(!strcmp(opt, "transmission-version")) {
    if (!strcmp(param, "external")) {
      /* we should probably only set this when transmission-external is set */
      as->transmission_version = AM_TRANSMISSION_EXTERNAL;
    } else if(param[0] == '1' && param[1] == '.' && param[2] == '2') {
      as->transmission_version = AM_TRANSMISSION_1_2;
    } else if(param[0] == '1' && param[1] == '.' && param[2] == '3') {
      as->transmission_version = AM_TRANSMISSION_1_3;
    } else {
      dbg_printf(P_ERROR, "Unknown parameter: %s=%s", opt, param);
    }
  } else if (!strcmp(opt, "transmission-external")) {
    set_path(param, &as->transmission_external);
    as->transmission_version = AM_TRANSMISSION_EXTERNAL;
  } else if(!strcmp(opt, "torrent-folder")) {
    set_path(param, &as->torrent_folder);
  } else if(!strcmp(opt, "statefile")) {
    set_path(param, &as->statefile);
  } else if(!strcmp(opt, "rpc-host")) {
    as->host = am_strdup(param);
  } else if(!strcmp(opt, "rpc-auth")) {
    as->auth = am_strdup(param);
  } else if(!strcmp(opt, "upload-limit")) {
    numval = parseUInt(param);
    if(numval > 0) {
      as->upspeed = (uint16_t)numval;
    } else {
      dbg_printf(P_ERROR, "Unknown parameter: %s=%s", opt, param);
    }
  } else if(!strcmp(opt, "rpc-port")) {
    numval = parseUInt(param);
    if (numval > 1024 && numval < 65535) {
      as->rpc_port = numval;
    } else if(numval != -1) {
      dbg_printf(P_ERROR, "RPC port must be an integer between 1025 and 65535, reverting to default (%d)\n\t%s=%s", AM_DEFAULT_RPCPORT, opt, param);
    } else {
      dbg_printf(P_ERROR, "Unknown parameter: %s=%s", opt, param);
    }
  } else if(!strcmp(opt, "interval")) {
    numval = parseUInt(param);
    if(numval > 0) {
      as->check_interval = numval;
    } else if(numval != -1) {
      dbg_printf(P_ERROR, "Interval must be 1 minute or more, reverting to default (%dmin)\n\t%s=%s", AM_DEFAULT_INTERVAL, opt, param);
    } else {
      dbg_printf(P_ERROR, "Unknown parameter: %s=%s", opt, param);
    }
  } else if(!strcmp(opt, "use-transmission")) {
    if(!strncmp(param, "0", 1) || !strncmp(param, "no", 2)) {
      as->use_transmission = 0;
    } else if(!strncmp(param, "1", 1) || !strncmp(param, "yes", 3)) {
      as->use_transmission = 1;
    } else {
      dbg_printf(P_ERROR, "Unknown parameter: %s=%s", opt, param);
    }
  } else if(!strcmp(opt, "start-torrents")) {
    if(!strncmp(param, "0", 1) || !strncmp(param, "no", 2)) {
      as->start_torrent = 0;
    } else if(!strncmp(param, "1", 1) || !strncmp(param, "yes", 3)) {
      as->start_torrent = 1;
    } else {
      dbg_printf(P_ERROR, "Unknown parameter for option '%s': '%s'", opt, param);
    }
  } else if(!strcmp(opt, "patterns")) {
    dbg_printf(P_ERROR, "the 'patterns' option is not supported any more, please use the 'filter' option instead!");
    result = FAILURE;
  } else if(!strcmp(opt, "filter")) {
    result = parseFilter(&as->filters, param);
  } else {
    dbg_printf(P_ERROR, "Unknown option: %s", opt);
  }

  return result;
}

PRIVATE int SkipWhitespace(const char *line, int line_pos, int * line_number) {
  int len = 0;

  /* skip whitespaces */
  if(line && *line) {
    len = strlen(line);

    while (isspace(line[line_pos]) && line_pos < len) {
      if(line[line_pos] == '\n') {
        *line_number += 1;
        dbg_printf(P_DBG, "skipping newline (line %d)", *line_number);
      }

      ++line_pos;
    }
  } else {
    line_pos = -1;
  }

  return line_pos;
}


/** \brief parse configuration file.
*
* \param[in,out] as Pointer to session handle
* \param[in] filename Path to the configuration file
* \return 0 if parsing was successful, -1 if an error occured.
*/
int parse_config_file(struct auto_handle *as, const char *filename) {
  FILE *fp = NULL;
  char *line = NULL;
  char opt[MAX_OPT_LEN + 1];
  char param[MAX_PARAM_LEN + 1];
  char c; /* for the "" and '' check */
  int line_num = 0;
  int line_pos = 0;
  int opt_pos;
  int param_pos;
  int parse_error = 0;
  struct stat fs;
  option_type type;

  if(stat(filename, &fs) == -1)  {
    return -1;
  }

  if ((fp = fopen(filename, "rb")) == NULL) {
    perror("fopen");
    return -1;
  }

  if ((line = am_malloc(fs.st_size + 1)) == NULL) {
    dbg_printf(P_ERROR, "Can't allocate memory for 'line': %s (%ldb)", strerror(errno), fs.st_size + 1);
    return -1;
  }

  if(fread(line, fs.st_size, 1, fp) != 1) {
    perror("fread");
    fclose(fp);
    am_free(line);
    return -1;
  }

  if(fp) {
    fclose(fp);
  }

  while(line_pos != fs.st_size) {
    line_pos = SkipWhitespace(line, line_pos, &line_num);

    if(line_pos >= fs.st_size) {
      break;
    }

    /* comment */
    if (line[line_pos] == '#') {
      ////dbg_printf(P_INFO2, "skipping comment (line %d)", line_num);
      while (line[line_pos] != '\n') {
        ++line_pos;
      }

      ++line_num;
      ++line_pos;  /* skip the newline as well */
      continue;
    }

    /* read option */
    for (opt_pos = 0; isprint(line[line_pos]) && line[line_pos] != ' ' &&
         line[line_pos] != '#' && line[line_pos] != '='; /* NOTHING */) {
      opt[opt_pos++] = line[line_pos++];
      if (opt_pos >= MAX_OPT_LEN) {
        dbg_printf(P_ERROR, "too long option at line %d", line_num);
        parse_error = 1;
      }
    }

    if (opt_pos == 0 || parse_error == 1) {
      dbg_printf(P_ERROR, "parse error at line %d (pos: %d)", line_num, line_pos);
      parse_error = 1;
      break;
    } else {
      opt[opt_pos] = '\0';
    }

    line_pos = SkipWhitespace(line, line_pos, &line_num);

    if(line_pos >= fs.st_size) {
      break;
    }

    /* check for '=' */
    if (line[line_pos++] != '=') {
         dbg_printf(P_ERROR, "Option '%s' needs a parameter (line %d)", opt, line_num);
      parse_error = 1;
      break;
    }

    line_pos = SkipWhitespace(line, line_pos, &line_num);

    if(line_pos >= fs.st_size) {
      break;
    }

    /* read the parameter */

    /* case 1: single string, no linebreaks allowed */
    if (line[line_pos] == '"' || line[line_pos] == '\'') {
      c = line[line_pos]; /* single or double quote */
      ++line_pos;  /* skip quote */
      parse_error = 0;
      for (param_pos = 0; line[line_pos] != c; /* NOTHING */) {
        if(line_pos < fs.st_size && param_pos < MAX_PARAM_LEN && line[line_pos] != '\n') {
          param[param_pos++] = line[line_pos++];
        } else {
               dbg_printf(P_ERROR, "Option %s has a too long parameter (line %d)",opt, line_num);
          parse_error = 1;
          break;
        }
      }

      if(parse_error == 0) {
        line_pos++;  /* skip the closing " or ' */
        type = CONF_TYPE_STRING;
      } else {
        break;
      }
    } else if (line[line_pos] == '{') { /* case 2: multiple items, linebreaks allowed */
      dbg_printf(P_DBG, "reading multiline param", line_num);
      ++line_pos;
      parse_error = 0;

      for (param_pos = 0; line[line_pos] != '}'; /* NOTHING */) {
        if(line_pos < fs.st_size && param_pos < MAX_PARAM_LEN) {
          param[param_pos++] = line[line_pos++];
          if(line[line_pos] == '\n') {
            line_num++;
          }
        } else {
          dbg_printf(P_ERROR, "Option %s has a too long parameter (line %d). Closing bracket missing?", opt, line_num);
          parse_error = 1;
          break;
        }
      }

      if(parse_error == 0) {
        line_pos++;  /* skip the closing '}' */
        type = CONF_TYPE_STRINGLIST;
      } else {
        break;
      }
    } else { /* Case 3: integers */
      parse_error = 0;
      for (param_pos = 0; isprint(line[line_pos]) && !isspace(line[line_pos])
        && line[line_pos] != '#'; /* NOTHING */) {
          param[param_pos++] = line[line_pos++];
          if (param_pos >= MAX_PARAM_LEN) {
            dbg_printf(P_ERROR, "Option %s has a too long parameter (line %d)", opt, line_num);
            parse_error = 1;
            break;
          }
      }

      if(parse_error == 0) {
        type = CONF_TYPE_INT;
      } else {
        break;
      }
    }

    param[param_pos] = '\0';
    dbg_printf(P_DBG, "[parse_config_file] option: %s", opt);
    dbg_printf(P_DBG, "[parse_config_file] param: %s (%d byte)", param, strlen(param));
    dbg_printf(P_DBG, "[parse_config_file] -----------------");

    if(set_option(as, opt, param, type) == FAILURE) {
      parse_error = 1;
      break;
    }

    line_pos = SkipWhitespace(line, line_pos, &line_num);

    if(line_pos >= fs.st_size) {
      break;
    }
  }

  am_free(line);

  return (parse_error == 1) ? -1 : 0;
}
