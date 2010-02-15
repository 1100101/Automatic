#include <assert.h>
#include <string.h>

#include "utils.h"
#include "output.h"
#include "prowl.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

int8_t verbose = P_MSG;

const char* correct_key = "";
const char* wrong_key = "132ieosdsd";

void testSendNotification(void) {
	int ret = 0;

  ret = sendProwlNotification(NULL, NULL, NULL);
  assert(ret == -1);
  ret = sendProwlNotification(wrong_key, NULL, NULL);
  assert(ret == -1);
  ret = sendProwlNotification(wrong_key, "Event", NULL);
  assert(ret == -401);
  ret = sendProwlNotification(wrong_key, NULL, "Desc");
  assert(ret == -401);
  ret = sendProwlNotification(wrong_key, "Event", "Desc");
  assert(ret == -401);
  ret = sendProwlNotification(correct_key, NULL, NULL);
  assert(ret == -1);
  ret = sendProwlNotification(correct_key, "Event", NULL);
  assert(ret == 1);
  ret = sendProwlNotification(correct_key, NULL, "Desc");
  assert(ret == 1);
  ret = sendProwlNotification(correct_key, "Event", "Desc");
  assert(ret == 1);
}

void testVerifyAPIKey(void) {
	int ret = 0;

  ret = verifyProwlAPIKey(NULL);
  assert(ret == -1);

  ret = verifyProwlAPIKey(wrong_key);
  assert(ret == -401);

  ret = verifyProwlAPIKey(correct_key);
  assert(ret == 1);

}

void testSendNotification2(void) {
	int ret = 0;

  ret = prowl_sendNotification(0, NULL, NULL);
  assert(ret == 0);
  ret = prowl_sendNotification(0, NULL, "file");
  assert(ret == 0);
  ret = prowl_sendNotification(0, wrong_key, NULL);
  assert(ret == 0);
  ret = prowl_sendNotification(0, wrong_key, "File");
  assert(ret == 0);
  ret = prowl_sendNotification(0, correct_key, NULL);
  assert(ret == 0);
  ret = prowl_sendNotification(0, correct_key, "File");
  assert(ret == 0);
  ret = prowl_sendNotification(PROWL_NEW_DOWNLOAD, wrong_key, NULL);
  assert(ret == 0);
  ret = prowl_sendNotification(PROWL_NEW_DOWNLOAD, wrong_key, "file");
  assert(ret == 0);
  ret = prowl_sendNotification(PROWL_DOWNLOAD_FAILED, wrong_key, NULL);
  assert(ret == 0);
  ret = prowl_sendNotification(PROWL_DOWNLOAD_FAILED, wrong_key, "file");
  assert(ret == 0);

  ret = prowl_sendNotification(PROWL_NEW_DOWNLOAD, correct_key, NULL);
  assert(ret == 1);
  ret = prowl_sendNotification(PROWL_NEW_DOWNLOAD, correct_key, "file");
  assert(ret == 1);
  ret = prowl_sendNotification(PROWL_DOWNLOAD_FAILED, correct_key, NULL);
  assert(ret == 1);
  ret = prowl_sendNotification(PROWL_DOWNLOAD_FAILED, correct_key, "file");
  assert(ret == 1);
}



int main(void) {
//	testVerifyAPIKey();
//  testSendNotification();
  testSendNotification2();
	return 0;
}
