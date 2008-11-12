/*
 * base64_test.c
 *
 *  Created on: Oct 20, 2008
 *      Author: aurich
 */


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "utils.h"
#include "output.h"
#include "base64.h"


int8_t verbose = P_NONE;

int main(void) {

	const char *dec_str = "The quick brown fox jumps over the lazy dog.";
	const char *enc_str = "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZy4=";

	char *encode = NULL, *decode = NULL;
	uint32_t enc_len, dec_len;

	encode = base64_encode(NULL, 47, &enc_len);
	assert(encode == NULL && enc_len == 0);

	encode = base64_encode(dec_str, strlen(dec_str), &enc_len);
	assert(enc_len == strlen(enc_str));
	assert(strcmp(encode, enc_str) == 0);
	decode = base64_decode(encode, enc_len, &dec_len);
	assert(dec_len == strlen(dec_str));
	assert(strcmp(decode, dec_str) == 0);
	am_free(encode);
	am_free(decode);
	return 0;
}
