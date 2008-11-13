#include <assert.h>
#include <string.h>

#include "regex.h"
#include "utils.h"
#include "output.h"

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

int8_t verbose = P_NONE;


void testIsMatch(void) {
	assert(isRegExMatch(NULL, NULL) == 0);
	assert(isRegExMatch(NULL, "test") == 0);
	assert(isRegExMatch("test", NULL) == 0);
	assert(isRegExMatch("test", "adc") == 0);
	assert(isRegExMatch("(a|b)c", "test") == 0);
	assert(isRegExMatch("test", "test") == 1);
	assert(isRegExMatch("(a|b)c", "abc") == 1);
	assert(isRegExMatch("(?!.*abc)def.*", "def abc") == 0);
	assert(isRegExMatch("(?!.*abc)def.*", "def ghi") == 1);
	assert(isRegExMatch("(?!.*abc)def.*", "def ghi") == 1);
	assert(isRegExMatch("(?!.*abc)def.*", "def ghi") == 1);
	assert(isRegExMatch("def.*ghi", "def xyz (ghi - rst)") == 1);
	assert(isRegExMatch("def.*ghi", "def xyz (abc - ghi - rst)") == 1);
	assert(isRegExMatch("def.*abc", "def xyz (ghi - rst)") == 0);
	assert(isRegExMatch("def.*abc", "def xyz (abc - ghi - rst)") == 1);
}


void testGetMatch(void) {
	const char* pattern1 = "Content-Disposition:\\s(inline|attachment);\\sfilename=\"(.+)\"$";
	const char* pattern2 = "\"result\":\\s\"(.+)\"\n";
	const char* string1 = "Content-Disposition: inline; filename=\"this.is.a.test-file.torrent\"";
	const char* string2 = "\"result\": \"success\"";
	char *res_str = NULL;

	res_str = getRegExMatch(NULL, NULL, 0);
	assert(res_str == NULL);

	res_str = getRegExMatch(pattern1, NULL, 0);
	assert(res_str == NULL);

	res_str = getRegExMatch(NULL, string1, 0);
	assert(res_str == NULL);

	res_str = getRegExMatch(pattern1, string1, 7);
	assert(res_str == NULL);

	res_str = getRegExMatch(pattern1, string1, 2);
	assert(strcmp(res_str, "this.is.a.test-file.torrent") == 0);
	am_free(res_str);

	res_str = getRegExMatch(pattern1, string2, 2);
	assert(res_str == NULL);
	am_free(res_str);

	res_str = getRegExMatch(pattern2, string2, 2);
	assert(res_str == NULL);
	am_free(res_str);

	res_str = getRegExMatch(pattern2, string2, 1);
	assert(strcmp(res_str, "success") == 0);
	am_free(res_str);

	res_str = getRegExMatch(pattern2, "\"result\": \"failure\"", 1);
	assert(strcmp(res_str, "failure") == 0);
	am_free(res_str);

	res_str = getRegExMatch(pattern2, "\"result\": \"duplicate torrent\"", 1);
	assert(strcmp(res_str, "duplicate torrent") == 0);
	am_free(res_str);
}

int main(void) {
	testIsMatch();
	testGetMatch();
	return 0;
}
