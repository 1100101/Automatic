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

int8_t verbose = P_NONE;

void testGetHTTP(void) {
	int ret = 1;
	HTTPResponse *response = NULL;


	//test invalid URL
	response = getHTTPData(NULL);
	assert(response == NULL);

	//test invalid URL 2
	response = getHTTPData("http://thisurldoesntexist.co.ge");
	assert(response);
	assert(response->responseCode != 200);
	assert(response->data == NULL);
	HTTPResponse_free(response);

	//test HTTP URL
	response = getHTTPData("http://www.binsearch.info/?action=nzb&33455941=1");
	assert(response && response->data);
	HTTPResponse_free(response);
}

int main(void) {
	testGetHTTP();
	return 0;
}
