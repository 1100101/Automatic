#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils.h"
#include "output.h"
#include "toasty.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

int8_t verbose = P_MSG;

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

const char* correct_key = "1880ccbd-2941-4164-9cea-fb7698a49562";
const char* wrong_key = "132ieosdsd";

static int
 testSendNotification(void) {
  int ret = 0;

  ret = sendToastyNotification(NULL, NULL, NULL);
  check(ret == -1);
  ret = sendToastyNotification(wrong_key, NULL, NULL);
  check(ret == -1);
  ret = sendToastyNotification(wrong_key, "Event", NULL);
  check(ret == -1);
  ret = sendToastyNotification(wrong_key, NULL, "Desc");
  check(ret == -1);
  ret = sendToastyNotification(wrong_key, "Event", "Desc");
  check(ret == -404);
  ret = sendToastyNotification(correct_key, NULL, NULL);
  check(ret == -1);
  ret = sendToastyNotification(correct_key, "Event", NULL);
  check(ret == -1);
  ret = sendToastyNotification(correct_key, NULL, "Desc");
  check(ret == -1);
  ret = sendToastyNotification(correct_key, "Event", "Desc");
  check(ret == 1);
  return 0;
}

static int
testSendNotification2(void) {
  int ret = 0;

  ret = toasty_sendNotification(0, NULL, NULL);
  check(ret == 0);
  ret = toasty_sendNotification(0, NULL, "file");
  check(ret == 0);
  ret = toasty_sendNotification(0, wrong_key, NULL);
  check(ret == 0);
  ret = toasty_sendNotification(0, wrong_key, "File");
  check(ret == 0);
  ret = toasty_sendNotification(0, correct_key, NULL);
  check(ret == 0);
  ret = toasty_sendNotification(0, correct_key, "File");
  check(ret == 0);
  ret = toasty_sendNotification(PROWL_NEW_DOWNLOAD, wrong_key, NULL);
  check(ret == 0);
  ret = toasty_sendNotification(PROWL_NEW_DOWNLOAD, wrong_key, "file");
  check(ret == 0);
  ret = toasty_sendNotification(PROWL_DOWNLOAD_FAILED, wrong_key, NULL);
  check(ret == 0);
  ret = toasty_sendNotification(PROWL_DOWNLOAD_FAILED, wrong_key, "file");
  check(ret == 0);

  ret = toasty_sendNotification(PROWL_NEW_DOWNLOAD, correct_key, NULL);
  check(ret == 1);
  ret = toasty_sendNotification(PROWL_NEW_DOWNLOAD, correct_key, "file");
  check(ret == 1);
  ret = toasty_sendNotification(PROWL_DOWNLOAD_FAILED, correct_key, NULL);
  check(ret == 1);
  ret = toasty_sendNotification(PROWL_DOWNLOAD_FAILED, correct_key, "file");
  check(ret == 1);
  return 0;
}



int main(void) {
  int i;

  i = testSendNotification();

  if(!i) {
    i = testSendNotification2();
  }

  return i;
}
