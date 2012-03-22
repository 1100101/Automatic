#include <pcre.h>
#include <string.h>
#include <assert.h>

#include "regex.h"
#include "output.h"
#include "utils.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

PRIVATE int32_t getPCRECaptureCount(const pcre * code) {
   int32_t count = 0;
   int32_t result;
   if(code != NULL) {
      if((result = pcre_fullinfo(code, NULL, PCRE_INFO_CAPTURECOUNT, &count)) != 0) {
         dbg_printf(P_ERROR, "[getPCRECaptureCount] pcre_fullinfo() returned %d", result);
         count = result;
      }
   }
   
   return count;
}

PRIVATE pcre* init_regex(const char* pattern) {
  int err;
  pcre *re = NULL;
  const char* errbuf = NULL;

  if(!pattern) {
    dbg_printf(P_ERROR, "[init_regex] Empty regex pattern!");
    return NULL;
  }

  dbg_printf(P_DBG, "[init_regex] Regular expression: %s", pattern);
  re = pcre_compile(pattern, PCRE_UTF8|PCRE_CASELESS|PCRE_EXTENDED, &errbuf, &err, NULL);

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
    dbg_printf(P_ERROR, "[isRegExMatch] Empty string!");
    return 0;
  }

  preg = init_regex(pattern);

  if(preg) {
    dbg_printf(P_DBG, "[isRegExMatch] Text to match against: %s", str);
    err = pcre_exec(preg, NULL, str, strlen(str), 0, 0, NULL, 0);
    dbg_printf(P_DBG, "[isRegExMatch] err=%d", err);
    if (!err) { /* regex matches */
      dbg_printf(P_MSG, "[isRegExMatch] '%s' matches '%s'", pattern, str);
      result = 1;
    } else {
      if(err != PCRE_ERROR_NOMATCH) {
        dbg_printf(P_ERROR, "[isRegExMatch] PCRE error: %d", err);
      }
      result = 0;
    }
    pcre_free(preg);
  }
  
  return result;
}

int32_t getRegExCaptureGroups(const char* pattern, const char* str, const char ***stringlist) {
  int32_t count = -1;
  pcre *result_preg = NULL;
  int32_t * ovec = NULL;
  int32_t captureGroupCount;
  int32_t ovecSize;

  if(!str || (str && strlen(str) == 0)) {
    dbg_printf(P_ERROR, "[getRegExCaptureGroups] Empty string!");
    return 0;
  }
  
  result_preg = init_regex(pattern);

  if(result_preg) {
    captureGroupCount = getPCRECaptureCount(result_preg);
    
    if(captureGroupCount < 0) {
      captureGroupCount = 9; // Fallback, in case pcre_fullinfo() returned an error
    }
    
    ovecSize = captureGroupCount > 0 ? (captureGroupCount + 1 ) * 3 : 0;
    
    if(ovecSize > 0) {
      ovec = (int32_t*)am_malloc(ovecSize * sizeof(int32_t));

      if(ovec == NULL) {
        dbg_printf(P_ERROR, "[getRegExCaptureGroups] malloc(offsets) failed!");
        pcre_free(result_preg);
        return 0;
      }
    }
    
    dbg_printf(P_INFO2, "[getRegExCaptureGroups] Text to match against: %s (%d byte)", str, strlen(str));

    count = pcre_exec(result_preg, NULL, str, strlen(str), 0, 0, ovec, ovecSize);
    if(count > 1) { /* regex matches */
      if(pcre_get_substring_list(str, ovec, count, stringlist) < 0) {
        dbg_printf(P_ERROR, "[getRegExCaptureGroups] Unable to obtain captured strings in regular expression.");
      }
    } else if(count < 0) {
      if(count == PCRE_ERROR_NOMATCH) {
        dbg_printf(P_DBG, "[getRegExCaptureGroups] No match");
      } else {
        dbg_printf(P_ERROR, "[getRegExCaptureGroups] regexec error: %d", count);
      }    
    }
    
    am_free(ovec);
    
    pcre_free(result_preg);
  }

  return count;
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
  char *result_str = NULL;
  const char **stringlist = NULL;

  if(!str || (str && strlen(str) == 0)) {
    dbg_printf(P_ERROR, "[getRegExMatch] Empty string!");
    return NULL;
  }

  count = getRegExCaptureGroups(pattern, str, &stringlist);  

  if(count > 0) {
    if(count > which_result) {
      result_str = am_strdup(stringlist[which_result]);
      pcre_free_substring_list(stringlist);
    } else {
      dbg_printf(P_ERROR, "[getRegexMatch] Requested substring (%d) exceeds number of captured substrings (%d)!", which_result, count);
    }
  }

  return strstrip(result_str);
}

char * performRegexReplace(const char* str, const char* pattern, const char* replace) {
  int match_count;
  const char **stringlist = NULL;
  int len,  pos;
#define MAX_SUBST_LEN 1024  
#define MAX_IDX_LEN 5
  char index[MAX_IDX_LEN];
  int ic = 0; /* index counter */
  int si = 0; /* subsitute index */
  int num;
  int fail = 0;
  char substitute[MAX_SUBST_LEN];
  
  memset(&substitute, 0, sizeof(substitute));

  if(!str || (str && strlen(str) == 0)) {
    dbg_printf(P_ERROR, "[regExReplace] Empty string!");
    return NULL;
  }
  
  if(!pattern || (pattern && strlen(pattern) == 0)) {
    dbg_printf(P_ERROR, "[regExReplace] Empty pattern!");
    return NULL;
  }
  
  if(!replace || (replace && strlen(replace) == 0)) {
    dbg_printf(P_ERROR, "[regExReplace] Empty substitution string!");
    return NULL;
  }
  
  match_count = getRegExCaptureGroups(pattern, str, &stringlist);
  if(match_count > 1) {
    len = strlen(replace);
    for(pos = 0; pos < len; ++pos) {
      if(replace[pos] == '\\') {
        pos++;
        
        if(!(replace[pos] >= 48 && replace[pos] <= 57)) {
          /* Not a substitution placeholder. */
          substitute[si++] = replace[pos - 1];
          substitute[si++] = replace[pos];
          continue;
        }
        
        while(replace[pos] >= '0' && replace[pos] <= '9') {
          if(ic > MAX_IDX_LEN - 2) {
            dbg_printf(P_ERROR, "[regExReplace] Substitute number must be between 1 and 99.");
            fail = 1;
            break;
          }
          
          index[ic++] =replace[pos++];
        }
        
        index[ic] = '\0';
        ic = 0;
        num = atoi(index);
        
        if (num < 1 || num > 99) {
          dbg_printf(P_ERROR, "[regExReplace] Substitute must be between 1 and 99.");
          fail = 1;
          break;
        }
        
        if(num < match_count) {
          int substring_len = strlen(stringlist[num]);
          memcpy(&substitute[si], stringlist[num], substring_len);
          si += strlen(stringlist[num]);
        } else {
          dbg_printf(P_ERROR, "[regExReplace] Substitution number greater than available capture groups.");
          fail = 1;
          break;
        }
			}
      
      substitute[si++] = replace[pos];
    }
    
    pcre_free_substring_list(stringlist);
    substitute[si] = '\0';
  } else if(match_count == 1) {
    dbg_printf(P_ERROR, "[regExReplace] No capture groups in regular expression! Nothing to replace");
    fail = 1;
  } else {
    fail = 1;
  }
  
  return (fail == 0) ? am_strdup(&substitute[0]) : NULL;
}
