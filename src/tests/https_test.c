/*
 * main.c*
 *
 *
 *  Created on: Oct 16, 2008
 *      Author: aurich
 */

#include "web.h"
#include "utils.h"
#include "file.h"
#include "output.h"


int testHTTPS(void) {
	int ret = 1;

	const char *url = "https://secure3.silverorange.com/rsstest/rss_with_ssl.xml";
	WebData *data = getHTTPData(url);
	if(data && data->response) {
		ret = saveFile("feed.xml", data->response->data, data->response->size);
	}
	if(ret == 0) {
		unlink("feed.xml");
	}
	WebData_free(data);
	return ret;
}

int main(void) {
	return testHTTPS();
}
