/*
 * main.c*
 *
 *
 *  Created on: Oct 16, 2008
 *      Author: aurich
 */

#include <assert.h>

#include "output.h"
#include "utils.h"
#include "web.h"
#include "urlcode.h"

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
 testGetHTTP(void) {
	int ret = 1;
	HTTPResponse *response = NULL;
   CURL *curl_session = NULL;


	//test invalid URL
	response = getHTTPData(NULL, NULL, &curl_session);
	check(response == NULL);

	//test invalid URL 2
	response = getHTTPData("http://thisurldoesntexist.co.ge", NULL, &curl_session);
	check(response == NULL);

	//test HTTP URL
	response = getHTTPData("http://www.heise.de/", NULL, &curl_session);
	check(response && response->data);
	HTTPResponse_free(response);
  closeCURLSession(curl_session);
   
  return 0;
}

int main(void) {
	return testGetHTTP();
}
