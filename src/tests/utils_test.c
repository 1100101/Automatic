/*
 * utils_test.c
 *
 *  Created on: Oct 20, 2008
 *      Author: aurich
 */


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "output.h"

int8_t verbose = P_NONE;

#define VERBOSE 1

static int test = 0;

#ifdef VERBOSE
  #define check( A ) \
    { \
        ++test; \
        if( A ){ \
            fprintf( stderr, "PASS test #%d (%s, %d)\n", test, __FILE__, __LINE__ ); \
        } else { \
            fprintf( stderr, "FAIL test #%d (%s, %d)\n", test, __FILE__, __LINE__ ); \
            return test; \
        } \
    }
#else
  #define check( A ) \
    { \
        ++test; \
        if( !( A ) ){ \
            fprintf( stderr, "FAIL test #%d (%s, %d)\n", test, __FILE__, __LINE__ ); \
            return test; \
        } \
    }
#endif

static int
testStringReplace(void) {
	char * ret = NULL;

  ret = am_replace_str(NULL, NULL, NULL);
  check(ret == NULL);

  ret = am_replace_str("test", NULL, NULL);
  check(ret == NULL);

  ret = am_replace_str(NULL, "test", NULL);
  check(ret == NULL);

  ret = am_replace_str(NULL, NULL, "test");
  check(ret == NULL);

  ret = am_replace_str("test", "test2", NULL);
  check(ret == NULL);

  ret = am_replace_str(NULL, "test", "test2");
  check(ret == NULL);

  ret = am_replace_str("test", NULL, "test2");
  check(ret == NULL);

  ret = am_replace_str("test", "test2", "");
  check(ret != NULL);
  check(strcmp(ret, "test") == 0);
  am_free(ret);

  ret = am_replace_str("test", "abc", "");
  check(ret != NULL);
  check(strcmp(ret, "test") == 0);
  am_free(ret);

  ret = am_replace_str("test", "es", "");
  check(ret != NULL);
  check(strcmp(ret, "tt") == 0);
  am_free(ret);

  ret = am_replace_str("test", "e", "oa");
  check(ret != NULL);
  check(strcmp(ret, "toast") == 0);
  am_free(ret);

  ret = am_replace_str("tester", "e", "oa");
  check(ret != NULL);
  check(strcmp(ret, "toastoar") == 0);
  am_free(ret);

  ret = am_replace_str("Planet\\ 9\\ From\\ Outer\\ Space", "\\ ", " ");
  check(ret != NULL);
  check(strcmp(ret, "Planet 9 From Outer Space") == 0);
  am_free(ret);
  return 0;
}


int main(void) {
  int i;
  
  i = testStringReplace();
  
  return i;
}
