#include <pcre.h>
#include <string.h>
#include <assert.h>

#include "regex.h"
#include "output.h"
#include "utils.h"

#define OVECCOUNT 30

static pcre* init_regex(const char* pattern) {
	int err;
	pcre *re = NULL;
	const char* errbuf = NULL;

	if(!pattern) {
		dbg_printf(P_ERROR, "[init_regex] Empty regex pattern!");
		return NULL;
	}

	dbg_printf(P_INFO2, "[init_regex] Regular expression: %s", pattern);
	re = pcre_compile(pattern, PCRE_CASELESS|PCRE_EXTENDED, &errbuf, &err, NULL);

	if(re == NULL) {
		dbg_printf(P_ERROR, "[init_regex] PCRE compilation failed at offset %d: %s", err, errbuf);
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

	if(!str) {
		dbg_printf(P_ERROR, "[isMatch] Empty string!");
		return 0;
	}

	preg = init_regex(pattern);

	if(preg) {
		dbg_printf(P_INFO2, "[isMatch] Text to match against: %s", str);
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
	int err, len;
	pcre *result_preg = NULL;
	char *result_str = NULL;
	int ovector[OVECCOUNT];

	if(!str) {
		dbg_printf(P_ERROR, "[getMatch] Empty string!");
		return NULL;
	}

	result_preg = init_regex(pattern);
	if(result_preg) {
		dbg_printf(P_INFO2, "[isMatch] Text to match against: %s", str);
		err = pcre_exec(result_preg, NULL, str, strlen(str),
										0, 0, ovector, OVECCOUNT);
		if(err > which_result) { /* regex matches */
			int i;
		  for (i = 0; i < err; i++) {
		    dbg_printf(P_DBG, "%2d: %.*s", i, ovector[2*i+1] - ovector[2*i], str + ovector[2*i]);
		  }
		  len = ovector[2 * which_result + 1] - ovector[2 * which_result];
			result_str = am_strndup(str + ovector[2 * which_result], len);
			dbg_printf(P_DBG, "[getMatch] result: '%s' (%d -> %d)", result_str, ovector[2 * which_result], ovector[2 * which_result + 1]);
		} else if(err < 0) {
			if(err == PCRE_ERROR_NOMATCH) {
				dbg_printf(P_INFO, "[getMatch] regexec didn't match. string was: %s", str);
			} else {
				dbg_printf(P_ERROR, "[getMatch] regexec error: %d", err);
			}
		}
		pcre_free(result_preg);
	}
	return result_str;
}
