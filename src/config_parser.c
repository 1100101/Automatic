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
#define MAX_OPT_LEN	50
#define MAX_PARAM_LEN	20000

struct option_item {
   char *str;
};

typedef struct option_item option_item_t;
/** \endcond */

PRIVATE void freeOptionItem(void* item) {
  if(item != NULL) {
    option_item_t* obj = (option_item_t*)item;
    am_free(obj->str);
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
      *dst = am_strdup(tmp);
      am_free(tmp);
    }
  }
}

PRIVATE int parseUInt(const char *str) {
  int is_num = 1;
  uint32_t i;

  for(i = 0; i < strlen(str); i++) {
    if(isdigit(str[i]) == 0) {
      is_num--;
    }
  }
  
  if(is_num == 1 && atoi(str) > 0) {
    return atoi(str);
  }
  
  return -1;
}

/* TODO: This does currently more than it should, clean this up. */
PRIVATE char* trim_obsolete(const char *str) {

  int tmp_pos;
  char c;
  char *retStr;
  char *tmp = (char*)am_malloc(MAX_PARAM_LEN+1);
  uint32_t line_pos = 0;
  uint32_t len = strlen(str);

  if(!tmp) {
    dbg_printf(P_ERROR, "[trim] calloc(MAX_PARAM_LEN) failed!");
    return NULL;
  }

  memset(tmp, 0, MAX_PARAM_LEN+1);

  while (isspace(str[line_pos])) {
    ++line_pos;
  }

  tmp_pos = 0;
  while(line_pos < len) {
    /* case 1: quoted strings */
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
      while(line_pos < len && str[line_pos] != '\n' && str[line_pos] != '\0') {
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


PRIVATE simple_list parseMultiOption(const char *str) {

  int tmp_pos;
  char c;
  char *tmp = (char*)am_malloc(MAX_PARAM_LEN+1);
  uint32_t line_pos = 0;
  uint32_t len = strlen(str);
  simple_list options = NULL;

  if(!tmp) {
    dbg_printf(P_ERROR, "[shorten] calloc(MAX_PARAM_LEN) failed!");
    return NULL;
  }

  while (isspace(str[line_pos])) {
    ++line_pos;
  }
  
  while(line_pos < len) {
    memset(tmp, 0, MAX_PARAM_LEN+1);
    tmp_pos = 0;
    
    /* case 1: quoted strings */
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
      while(line_pos < len && str[line_pos] != '\n' && str[line_pos] != '\0') {
        tmp[tmp_pos++] = str[line_pos++];
      }
    }

    /* A line is finished, end it with a null terminator */
    tmp[tmp_pos] = '\0';

    /* store the line in our list */
    if(tmp_pos != 0) {
      option_item_t* i = (option_item_t*)am_malloc(sizeof(option_item_t));
      
      if(i != NULL) {
        i->str = am_strdup(tmp);
        addItem(i, &options);
      }
    }

    /* skip any additional whitespace at the end of the line */
    while (isspace(str[line_pos])) {
      ++line_pos;
    }
  }

  assert(strlen(tmp) < MAX_PARAM_LEN);
  am_free(tmp);
  
  return options;
}


PRIVATE int parseSubOption(char* line, char **option, char **param) {
  const char *subopt_delim = "=>";
  uint32_t i = 0;

  *option = NULL;
  *param  = NULL;

  assert(line && *line);

  while(line[i] != '\0') {
    if(line[i]   == subopt_delim[0] && line[i+1] == subopt_delim[1]) {
      *option = am_strndup(line, i-1);
      *param  = am_strdup(line + i + strlen(subopt_delim));
      break;
    }
    
    i++;
  }

  if(*option && *param)
    return 0;
  else
    return -1;
}

PRIVATE int parseFilter(am_filters *patlist, const char* match) {
  char *line = NULL, *option = NULL, *param = NULL;
  am_filter filter = NULL;
  int result = SUCCESS; /* be optimistic */
  simple_list option_list = NULL;  
  NODE * current = NULL;
  option_item_t *opt_item = NULL;
  
  option_list = parseMultiOption(match);
  current = option_list;

  while (current != NULL) {
    opt_item = (option_item_t*)current->data;
    if(opt_item != NULL) {
      if(!filter) {
        filter = filter_new();
        assert(filter && "filter_new() failed!");
      }    
       
      if(parseSubOption(opt_item->str, &option, &param) == 0) {
        if(!strncmp(option, "pattern", 7)) {
          filter->pattern = trim(param);
        } else if(!strncmp(option, "folder", 6)) {
          filter->folder = trim(param);
        } else {
          dbg_printf(P_ERROR, "Unknown suboption '%s'!", option);
        }
        
        am_free(option);
        am_free(param);
      } else {
        dbg_printf(P_ERROR, "Invalid suboption string: '%s'!", opt_item->str);
      }
    } else {
      assert(0 && "opt_item == NULL");
    }
    
    current = current->next;
  }

  if(filter && filter->pattern) {
    filter_add(filter, patlist);
  } else {
    dbg_printf(P_ERROR, "Invalid filter: '%s'", match);
    result = FAILURE;
  }
 
  if(option_list != NULL) {
    freeList(&option_list, freeOptionItem);
  }
  
  return result;
}

PRIVATE int addPatterns_old(am_filters *patlist, const char* strlist) {
  simple_list option_list = NULL;  
  NODE * current = NULL;
  option_item_t *opt_item = NULL;

  assert(patlist != NULL);
  
  option_list = parseMultiOption(strlist);
  current = option_list;

  while (current != NULL) {
    opt_item = (option_item_t*)current->data;
    if(opt_item != NULL) {
      am_filter pat = filter_new();
      assert(pat != NULL);
      pat->pattern = strdup(opt_item->str);
      filter_add(pat, patlist);
    } else {
      assert(0 && "opt_item == NULL");
    }
    
    current = current->next;
  }
  
  if(option_list != NULL) {
    freeList(&option_list, freeOptionItem);
  }
  
  return SUCCESS;
}

PRIVATE void parseCookiesFromURL(rss_feed* feed) {
  const char* result_regex = ":COOKIE:(.+)";

  assert(feed && feed->url && *feed->url);

  feed->cookies = getRegExMatch(result_regex, feed->url, 1);
}

PRIVATE int parseFeed(rss_feeds *feeds, const char* feedstr) {
  char *option = NULL, *param = NULL;
  rss_feed* feed = NULL;
  int result = SUCCESS; /* be optimistic */
  simple_list option_list = NULL;  
  NODE * current = NULL;
  option_item_t *opt_item = NULL;

  option_list = parseMultiOption(feedstr);
  current = option_list;
  
  while (current != NULL) {
    opt_item = (option_item_t*)current->data;
    if(opt_item != NULL) {
      if(!feed) {
        feed = feed_new();
        assert(feed && "feed_new() failed!");
      }
      
      if(parseSubOption(opt_item->str, &option, &param) == 0) {
        if(!strncmp(option, "url", 3)) {
          feed->url = trim(param);
        } else if(!strncmp(option, "cookies", 6)) {
          feed->cookies = trim(param);
        } else {
          dbg_printf(P_ERROR, "Unknown suboption '%s'!", option);
        }
        
        am_free(option);
        am_free(param);
      } else {
        dbg_printf(P_ERROR, "Invalid suboption string: '%s'!", opt_item->str);
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

    feed->id = listCount(*feeds);
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

PRIVATE int getFeeds(NODE **head, const char* strlist) {
  simple_list option_list = NULL;  
  NODE * current = NULL;
  option_item_t *opt_item = NULL;

  assert(head != NULL);

  option_list = parseMultiOption(strlist);
  current = option_list;
  
  while (current != NULL) {
    opt_item = (option_item_t*)current->data;
    if(opt_item != NULL) {
      rss_feed* feed = feed_new();
      assert(feed && "feed_new() failed!");
      feed->url = strdup(opt_item->str);
      feed->id  = listCount(*head);
      
      /* Maybe the cookies are encoded within the URL */
      parseCookiesFromURL(feed);
      feed_add(feed, head);
    } else {
      assert(0 && "opt_item == NULL");
    }
    
    current = current->next;
  }

  if(option_list != NULL) {
    freeList(&option_list, freeOptionItem);
  }

  return 0;
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
  dbg_printf(P_INFO2, "%s=%s (type: %d)", opt, param, type);

  assert(as != NULL);
  if(!strcmp(opt, "url")) {
    getFeeds(&as->feeds, param);
  } else if(!strcmp(opt, "feed")) {
    parseFeed(&as->feeds, param);
  } else if(!strcmp(opt, "transmission-home")) {
    set_path(param, &as->transmission_path);
  } else if(!strcmp(opt, "prowl-apikey")) {
    as->prowl_key = am_strdup(param);
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
    addPatterns_old(&as->filters, param);
  } else if(!strcmp(opt, "filter")) {
    parseFilter(&as->filters, param);
  } else {
    dbg_printf(P_ERROR, "Unknown option: %s", opt);
  }
  
  return 0;
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
  char *param = NULL;
  char erbuf[100];
  char c;		/* for the "" and '' check */
  int line_num = 0;
  int line_pos;	/* line pos */
  int opt_pos;	/* opt pos */
  int param_pos;	/* param pos */
  int parse_error = 0;
  int opt_good = 0;
  int param_good = 0;
  struct stat fs;
  option_type type;

  if(stat(filename, &fs) == -1)  {
    return -1;
  }
  dbg_printf(P_INFO2, "Configuration file size: %d", fs.st_size);

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
  line_pos = 0;

  param = (char*)am_malloc(MAX_PARAM_LEN + 1);
  if(!param) {
    dbg_printf(P_ERROR, "Can't allocate memory for 'param': %s (%ldb)", strerror(errno), MAX_PARAM_LEN + 1);
    am_free(line);
    return -1;
  }

  while(line_pos != fs.st_size) {
    /* skip whitespaces */
    while (isspace(line[line_pos])) {
      if(line[line_pos] == '\n') {
        dbg_printf(P_INFO2, "skipping newline (line %d)", line_num);
        line_num++;
      }
      ++line_pos;
    }

    if(line_pos >= fs.st_size) {
      break;
    }

    /* comment */
    if (line[line_pos] == '#') {
      dbg_printf(P_INFO2, "skipping comment (line %d)", line_num);
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
          opt_good = 0;
        }
    }
    if (opt_pos == 0 || parse_error == 1) {
      dbg_printf(P_ERROR, "parse error at line %d (pos: %d)", line_num, line_pos);
      parse_error = 1;
      break;
    } else {
      opt[opt_pos] = '\0';
      opt_good = 1;
    }
    /* skip whitespaces */
    while (isspace(line[line_pos])) {
      if(line[line_pos] == '\n') {
        line_num++;
        dbg_printf(P_INFO2, "skipping newline (line %d)", line_num);
      }
      ++line_pos;
    }

    if(line_pos >= fs.st_size) {
      break;
    }

    /* check for '=' */
    if (line[line_pos++] != '=') {
      snprintf(erbuf, sizeof(erbuf), "Option '%s' needs a parameter (line %d)", opt, line_num);
      parse_error = 1;
      break;
    }

    /* skip whitespaces */
    while (isspace(line[line_pos])) {
      if(line[line_pos] == '\n') {
        line_num++;
        dbg_printf(P_INFO2, "skipping newline (line %d)", line_num);
      }
      ++line_pos;
    }

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
          snprintf(erbuf, sizeof(erbuf), "Option %s has a too long parameter (line %d)\n",opt, line_num);
          parse_error = 1;
          break;
        }
      }
      if(parse_error == 0) {
        line_pos++;	/* skip the closing " or ' */
        type = CONF_TYPE_STRING;
      } else {
        break;
      }
      /* case 2: multiple items, linebreaks allowed */
    } else if (line[line_pos] == '{') {
      dbg_printf(P_INFO2, "reading multiline param", line_num);
      ++line_pos;
      parse_error = 0;
      for (param_pos = 0; line[line_pos] != '}'; /* NOTHING */) {
        if(line_pos < fs.st_size && param_pos < MAX_PARAM_LEN) {
          param[param_pos++] = line[line_pos++];
          if(line[line_pos] == '\n')
            line_num++;
        } else {
          snprintf(erbuf, sizeof(erbuf), "Option %s has a too long parameter (line %d)\n", opt, line_num);
          parse_error = 1;
          break;
        }
      }
      dbg_printf(P_INFO2, "multiline param: param_good=%d", param_good);
      if(parse_error == 0) {
        line_pos++;	/* skip the closing '}' */
        type = CONF_TYPE_STRINGLIST;
      } else {
        break;
      }
      /* Case 3: integers */
    } else {
      parse_error = 0;
      for (param_pos = 0; isprint(line[line_pos]) && !isspace(line[line_pos])
        && line[line_pos] != '#'; /* NOTHING */) {
          param[param_pos++] = line[line_pos++];
          if (param_pos >= MAX_PARAM_LEN) {
            snprintf(erbuf, sizeof(erbuf), "Option %s has a too long parameter (line %d)\n", opt, line_num);
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
    dbg_printf(P_INFO2, "[parse_config_file] option: %s", opt);
    dbg_printf(P_INFO2, "[parse_config_file] param: %s (%d byte)", param, strlen(param));
    dbg_printf(P_INFO2, "[parse_config_file] -----------------");
    set_option(as, opt, param, type);

    /* skip whitespaces */
    while (isspace(line[line_pos])) {
      if(line[line_pos] == '\n')
        line_num++;
      ++line_pos;
    }
    
    if(line_pos >= fs.st_size) {
      break;
    }
  }

  am_free(line);
  am_free(param);

  return (parse_error == 1) ? -1 : 0;
}

