/*
 * base64_test.c
 *
 *  Created on: Oct 20, 2008
 *      Author: aurich
 */


#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "output.h"
#include "base64.h"

int main(void) {

	const char *test_str = "test-string";
	const char *enc_str = "dGVzdC1zdHJpbmcK";

	char *encode = NULL;
	uint32_t len, res;
	encode = base64_encode(test_str, strlen(test_str), &len);
	dbg_printf(P_MSG, "result:   %s (%d)", encode, len);
	dbg_printf(P_MSG, "expected: %s (%d)", enc_str, strlen(enc_str));
	assert(len == strlen(enc_str));
	res = strcmp(encode, enc_str);
	assert(res == 0);
	am_free(encode);
	return 0;
}
