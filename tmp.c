#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libxml/nanohttp.h>

#define MAXPATHLEN 300

int main(int argc, char **argv) {
	void *ctxt = NULL;

	if(argc != 2) {
		printf("Usage: %s <url>\n", argv[0]);
		exit(1);
	}
	xmlNanoHTTPInit();
	ctxt = xmlNanoHTTPOpen(argv[1], NULL);
	if(!ctxt) {
		printf("Could not open HTTP connection\n");
	} else {
		printf("return code: %d\n", xmlNanoHTTPReturnCode(ctxt));
	}
	xmlNanoHTTPClose(ctxt);
	xmlNanoHTTPCleanup();
	return 0;
}

