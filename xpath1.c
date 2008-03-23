
#define MEMWATCH 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <regex.h>
#include <sys/stat.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/nanohttp.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "list.h"
#include "memwatch.h"

#define MAX_BUFFER 20000
#define MSGSIZE_MAX 300

static linked_list rss_items = NULL;
static linked_list regex_items = NULL;

static int verbose = 2;

enum debug_type {
	P_MSG,
	P_INFO,
	P_INFO2,
	P_ERROR
};
typedef enum debug_type debug_type;

void dbg_printf(debug_type type, const char *format, ...) {
	va_list va;
   char tmp[MSGSIZE_MAX];
	int print_msg = 0;
	switch(type)
		case P_ERROR: {
			print_msg = 1;
			break;
		case P_MSG: {
			if(verbose >= 1)
				print_msg = 1;
			break;
		}
		case P_INFO: {
			if(verbose >= 2)
				print_msg = 1;
			break;
		}
		default: {
			if(verbose > 2)
				print_msg = 1;
			break;
		}
	}
	if(print_msg) {
		va_start(va, format);
		vsnprintf(tmp, MSGSIZE_MAX, format, va);
		va_end(va);
		tmp[MSGSIZE_MAX-2] = '\n';
		tmp[MSGSIZE_MAX-1] = 0;
		fprintf(stdout,"%s", tmp);
	}
}

void print_xpath_nodes(xmlNodeSetPtr nodes) {
	xmlNodePtr cur, child;
	xmlChar *textNode;
	list_elem newItem;
	int size, i, len;
	int name_set, url_set;
	char *str;

	size = (nodes) ? nodes->nodeNr : 0;

	dbg_printf(P_MSG, "Result (%d nodes):\n", size);
	for(i = 0; i < size; ++i) {
		assert(nodes->nodeTab[i]);
		if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
			cur = nodes->nodeTab[i];
			if(cur->children) { /* != NULL && cur->children->type == XML_TEXT_NODE) { */
				child = cur->children;
				url_set = 0;
				name_set = 0;
				while(child) {
					if((strcmp((char*)child->name, "title") == 0) ||
						(strcmp((char*)child->name, "link" ) == 0)) {
						textNode = xmlNodeGetContent(child->children);
						len = strlen((char*)textNode);
						dbg_printf(P_INFO2, "Found text node '%s'\n", textNode);
						if(strlen((char*)textNode) > MAX_LEN-1) {
							dbg_printf(P_ERROR, "Error:  text node larger than 100 characters: %s\n", textNode);
							continue;
						}
						str = malloc(len + 1);
						if(str) {
							dbg_printf(P_INFO, "allocated %d bytes for 'str'\n", len+1);
							strcpy(str, (char*)textNode);
							if(strcmp((char*)child->name, "title") == 0) {
								newItem.name = str;
								name_set = 1;
							} else if(strcmp((char*)child->name, "link") == 0) {
								newItem.url = str;
								url_set = 1;
							}
						} else {
							perror("malloc");
							name_set = 0;
						}
						/*xmlFree(textNode);*/
					}
					if(name_set && url_set) {
						break;
					}
					child = child->next;
				}
				if(name_set && url_set) {
					addItem(newItem, &rss_items);
				}
				child = NULL;
				cur = NULL;
			}
		} else {
			cur = nodes->nodeTab[i];
			dbg_printf(P_MSG, "= node \"%s\": type %d\n", cur->name, cur->type);
		}
	}
}

int execute_xpath_expression(const char* buffer, int size, const xmlChar* xpathExpr) {
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;

	/* Init libxml */
	xmlInitParser();

	assert(buffer);
	assert(xpathExpr);

	/* Load XML document */
	doc = xmlParseMemory(buffer, size);
	if (doc == NULL) {
		dbg_printf(P_ERROR, "Error: unable to parse buffer\n");
		return(-1);
	}

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
		dbg_printf(P_ERROR, "Error: unable to create new XPath context\n");
		xmlFreeDoc(doc);
		return(-1);
	}

	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
	if(xpathObj == NULL) {
		dbg_printf(P_ERROR, "Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		return(-1);
	}

	/* Print results */
	print_xpath_nodes(xpathObj->nodesetval);

	/* Cleanup */
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	/* Shutdown libxml */
	xmlCleanupParser();
	return 0;
}

char* download_xml(const char *url, int *plength) {
	void *ctxt = NULL;
	char *content = NULL;
	int content_length = 0;
	int res;

	if (plength) {
		*plength = 0;
	}

	xmlNanoHTTPInit();
	ctxt = xmlNanoHTTPOpen(url, NULL);
	if(!ctxt) {
		dbg_printf(P_ERROR, "Could not open HTTP connection\n");
	} else if(xmlNanoHTTPReturnCode(ctxt) == 200) {
		dbg_printf(P_MSG, "HTTP response: 200\n");
		content_length = xmlNanoHTTPContentLength(ctxt);
		dbg_printf(P_INFO, "Length: %d\n", content_length);
		if(content_length > 0) {
			content = malloc(MAX_BUFFER);
			if(!content) {
				perror("malloc");
				content = NULL;
			} else {
				dbg_printf(P_INFO, "[download_xml] allocated %d bytes for 'content'\n", MAX_BUFFER);
				res = xmlNanoHTTPRead(ctxt, content, MAX_BUFFER);
				dbg_printf(P_INFO, "xmlNanoHTTPRead: %d\n", res);
				if(res < MAX_BUFFER) {
					content = realloc(content, res);
					if(!content) {
						perror("realloc");
					}
					if (plength) {
						*plength = res;
					}
				} else if(res == MAX_BUFFER) {
					dbg_printf(P_ERROR, "HTTP response larger than 65k! Buffer too small\n");
					free(content);
					dbg_printf(P_INFO, "[download_xml] freed 'content'\n");
				} else {
					dbg_printf(P_ERROR, "xmlNanoHTTPRead failed: %d\n", res);
					free(content);
					dbg_printf(P_INFO, "[download_xml] freed 'content'\n");
				}
			}
		} else {
			dbg_printf(P_ERROR, "Could not determine content length\n");
		}
	} else {
		dbg_printf(P_ERROR, "HTTP response: %d\n", xmlNanoHTTPReturnCode(ctxt));
	}
	if(ctxt) {
		xmlNanoHTTPClose(ctxt);
	}
	xmlNanoHTTPCleanup();
	xmlMemoryDump();
	return content;
}

int read_file(const char *filename) {
	FILE *f;
	char *buf, *p;
	struct stat fs;
	size_t fsize, read_bytes, count = 0;
	list_elem re;
	unsigned int len;

	if(stat(filename, &fs) == -1) {
		perror("Unable to determine file size of config file");
		return 0;
	}
	fsize = fs.st_size;
	f = fopen(filename, "r");
	if(f == NULL) {
		perror("fopen");
		return 0;
	}

	buf = malloc(fsize);
	if(!buf) {
		perror("malloc");
		return -1;
	}
	dbg_printf(P_INFO, "[read_file] allocated %d bytes for 'buf'\n", fsize);
	read_bytes = fread(buf, 1, fsize, f);
	fclose(f);
	p = strtok(buf, "\n");
	while (p) {
		len = strlen(p);
		if(len > MAX_LEN-1) {
			dbg_printf(P_ERROR, "Error: RegExp larger than 100 characters: %s\n", p);
			continue;
		}
		re.name = malloc(len + 1);
		if(re.name) {
			dbg_printf(P_INFO, "[read_file] allocated %d bytes for 're.name'\n", len + 1);
			strcpy(re.name, p);
			addItem(re, &regex_items);
			count++;
		} else {
			perror("malloc");
		}
		p = strtok(NULL, "\n");
	}
	free(buf);
	dbg_printf(P_INFO, "[read_file] freed 'buf'\n");
	return count;
}

void find_shows() {
	int err;
	regex_t preg;
	size_t len;
	char erbuf[100], *regex_str, *rss_str;

	NODE *current_regex = regex_items;
	while (current_regex != NULL) {
		NODE *current_rss_item = rss_items;
/*		regex_str = malloc(strlen(current_regex->elem.name) + 1);
		if(!regex_str) {
			dbg_printf(P_ERROR, "Error: Could not allocate memory for regex_str\n");
			current_regex = current_regex->pNext;
			continue;
		}
		dbg_printf(P_INFO, "[find_shows] allocated %d bytes for 'regex_str'\n", strlen(current_regex->elem.name) + 1);
		strcpy(regex_str, current_regex->elem.name);
		err = regcomp(&preg, regex_str, REG_EXTENDED|REG_ICASE);*/
		err = regcomp(&preg, current_regex->elem.name, REG_EXTENDED|REG_ICASE);
		if(err) {
			dbg_printf(P_ERROR, "regcomp: Error compiling regular expression: %d\n", err);
			current_regex = current_regex->pNext;
			/*free(regex_str);
			dbg_printf(P_INFO, "[find_shows] freed 'regex_str'\n");*/
			continue;
		}
		while (current_rss_item != NULL) {
/*			rss_str = malloc(strlen(current_rss_item->elem.name) + 1);
			if(!rss_str) {
				dbg_printf(P_ERROR, "Error: Could not allocate memory for rss_str\n");
				current_rss_item = current_rss_item->pNext;
				continue;
			}
			dbg_printf(P_INFO, "[find_shows] allocated %d bytes for 'rss_str'\n", strlen(current_rss_item->elem.name) + 1);
			strcpy(rss_str, current_rss_item->elem.name);
			err = regexec(&preg, rss_str, 0, NULL, 0);*/
			err = regexec(&preg, current_rss_item->elem.name, 0, NULL, 0);
			if(err) {
				len = regerror(err, &preg, erbuf, sizeof(erbuf));
				if(strcmp(erbuf, "No match") == 0) {
					/* Ignore non-matches */
				} else {
					dbg_printf(P_ERROR, "regexec error: %s\n", erbuf);
				}
			} else {
				dbg_printf(P_MSG, "match: %s\n", current_rss_item->elem.name);
			}
			current_rss_item = current_rss_item->pNext;
			/*free(rss_str);
			dbg_printf(P_INFO, "[find_shows] freed 'rss_str'\n");*/
		}
		current_regex = current_regex->pNext;
		regfree(&preg);
/*		free(regex_str);
		dbg_printf(P_INFO, "[find_shows] freed 'regex_str'\n");*/
	}
}

void free_nodes(linked_list list) {
	NODE *current = list;
	while (current != NULL) {
		if(current->elem.name)
			free(current->elem.name);
			dbg_printf(P_INFO, "[free_nodes] freed 'elem.name'\n");
		if(current->elem.url)
			free(current->elem.url);
			dbg_printf(P_INFO, "[free_nodes] freed 'elem.url'\n");
		current = current->pNext;
	}
}


int main(int argc, char **argv) {
	char *data = NULL;
	int data_size = 0;
	int i;
	const char *url = "http://tvrss.net/feed/eztv/";
	LIBXML_TEST_VERSION

	i = read_file("test.conf");
	if(i <= 0) {
		dbg_printf(P_ERROR, "Error: No regular expressions in test.conf\n");
		exit(1);
	}
	dbg_printf(P_MSG, "%d regular expressions found\n", listCount(regex_items));
	data = download_xml(url, &data_size);
	if(data) {
		dbg_printf(P_INFO, "size: %d\n", data_size);
		if(data_size > 0) {
			execute_xpath_expression(data, data_size, (xmlChar*)"//item");
		}
		free(data);
		dbg_printf(P_INFO, "[main] freed 'data'\n");
		find_shows();
	}
	free_nodes(rss_items);
	free_nodes(regex_items);
	freeList(&rss_items);
	freeList(&regex_items);
	return 0;
}
