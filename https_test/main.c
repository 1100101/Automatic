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

int main(void) {

	const char *url = "https://secure3.silverorange.com/rsstest/rss_with_ssl.xml";
	WebData *data = getHTTPData(url);
	if(data && data->response) {
		dbg_printf(P_MSG, "received %d bytes of data from '%s'", data->response->size, url);
		saveFile("feed.xml", data->response->data, data->response->size);
	}
	WebData_free(data);
	return 0;
}
