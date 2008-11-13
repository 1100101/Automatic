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
	WebData *data = NULL;


	//test invalid URL
	data = getHTTPData(NULL);
	assert(data == NULL);

	//test HTTPS URL
	data = getHTTPData("https://secure3.silverorange.com/rsstest/rss_with_ssl.xml");
	assert(data && data->response);
	WebData_free(data);
}

int main(void) {
	testGetHTTP();
	return 0;
}
