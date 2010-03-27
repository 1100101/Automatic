#include <pcre.h>
#include <string.h>
#include <assert.h>

#include "regex.h"
#include "output.h"
#include "utils.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define OVECCOUNT 30

static pcre* init_regex(const char* pattern) {
  int err;
  pcre *re = NULL;
  const char* errbuf = NULL;

  if(!pattern) {
    dbg_printf(P_ERROR, "[init_regex] Empty regex pattern!");
    return NULL;
  }

  dbg_printf(P_DBG, "[init_regex] Regular expression: %s", pattern);
  re = pcre_compile(pattern, PCRE_CASELESS|PCRE_EXTENDED, &errbuf, &err, NULL);

  if(re == NULL) {
    dbg_printf(P_ERROR, "[init_regex] PCRE compilation failed at offset %d: %s (pattern: %s)", err, errbuf, pattern);
  }
  return re;
}


/** \brief Check whether a given string matches a pattern using regular expressions
 *
 * \param pattern The regular expression
 * \param str The string
 *
 */
uint8_t isRegExMatch(const char* pattern, const char* str) {
  int err;
  pcre *preg = NULL;
  uint8_t result = 0;

  if(!str || (str && strlen(str) == 0)) {
    dbg_printf(P_ERROR, "[isMatch] Empty string!");
    return 0;
  }

  preg = init_regex(pattern);

  if(preg) {
    dbg_printf(P_DBG, "[isMatch] Text to match against: %s", str);
    err = pcre_exec(preg, NULL, str, strlen(str), 0, 0, NULL, 0);
    dbg_printf(P_DBG, "[getMatch] err=%d", err);
    if (!err) { /* regex matches */
      result = 1;
    } else {
      if(err != PCRE_ERROR_NOMATCH) {
        dbg_printf(P_ERROR, "[isMatch] PCRE error: %d", err);
      }
      result = 0;
    }
    pcre_free(preg);
  }
  return result;
}


/** \brief Retrieve a matched string from a regular expression comparison
 *
 * \param pattern The regular expression
 * \param str The string to compare against
 * \param which_result The number of the submatch that shall be returned
 *
 * \c which_result can be used to retrieve a specifice submatch of a comparison.
 * When set to 0, and the string matches the pattern, the complete match is returned.
 * Higher values for \c which_result should only be used if the pattern contains groups.
 */
char* getRegExMatch(const char* pattern, const char* str, uint8_t which_result) {
  int count;
  pcre *result_preg = NULL;
  char *result_str = NULL;
  const char **stringlist;
  int offsets[OVECCOUNT];

  if(!str || (str && strlen(str) == 0)) {
    dbg_printf(P_ERROR, "[getMatch] Empty string!");
    return NULL;
  }

  result_preg = init_regex(pattern);
  if(result_preg) {
    dbg_printf(P_INFO2, "[isMatch] Text to match against: %s (%d byte)", str, strlen(str));
    count = pcre_exec(result_preg, NULL, str, strlen(str), 0, 0, offsets, OVECCOUNT);
    if(count > which_result) { /* regex matches */
      //~ int i;
      //~ for (i = 0; i < count; i++) {
        //~ dbg_printf(P_DBG, "%2d: %.*s", i, offsets[2*i+1] - offsets[2*i], str + offsets[2*i]);
      //~ }
      //~ len = offsets[2 * which_result + 1] - offsets[2 * which_result];
      //~ result_str = am_strndup(str + offsets[2 * which_result], len);
      //~ dbg_printf(P_DBG, "[getMatch] result: '%s' (%d -> %d)", result_str, offsets[2 * which_result], offsets[2 * which_result + 1]);
      if(pcre_get_substring_list(str, offsets, count, &stringlist) < 0) {
        dbg_printf(P_ERROR, "[getMatch] Unable to obtain captured strings in regular expression.");
      } else {
        int i;
        for (i = 0; i < count; i++) {
          dbg_printf(P_DBG, "%2d: %s", i, stringlist[i]);
        }
        result_str = am_strdup(stringlist[which_result]);
        pcre_free_substring_list(stringlist);
      }
    } else if(count < 0) {
      if(count == PCRE_ERROR_NOMATCH) {
        dbg_printf(P_DBG, "[getMatch] No match");
      } else {
        dbg_printf(P_ERROR, "[getMatch] regexec error: %d", count);
      }
    }
    pcre_free(result_preg);
  }
  return strstrip(result_str);
}
