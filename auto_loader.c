/* #define NODAEMON */

#define LOCK_FILE	"/tmp/auto_loader.pid"
#define LOG_FILE	"/tmp/auto_loader.log"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/nanohttp.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "list.h"

#define MAX_BUFFER 128 * 1024   /* max. file size for xml/torrent = 128k */
#define MSGSIZE_MAX 300
#define MAX_ITEMS 10

void shutdown_daemon(void);
void cleanup_list(NODE **list);
char* getlogtime_str( char * buf, int buflen );
void freeItem(NODE *item);

static linked_list rss_items = NULL;
static linked_list regex_items = NULL;
static linked_list bucket = NULL;

list_elem newItem() {
	list_elem i;
	i.name = NULL;
	i.url = NULL;
	return i;
}

enum debug_type {
	P_ERROR,
	P_MSG,
	P_INFO,
	P_INFO2
};
typedef enum debug_type debug_type;

static int verbose = P_MSG;

void dbg_printf(debug_type type, const char *format, ...) {
	va_list va;
   char tmp[MSGSIZE_MAX];
	int print_msg = 0;
	FILE *logfile;
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
		tmp[MSGSIZE_MAX-1] = '\0';
		logfile = fopen(LOG_FILE,"a");
		if(logfile) {
#ifdef NODAEMON
			fprintf(stdout,"%s", tmp);
#else
			fprintf(logfile,"%s", tmp);
#endif
			fclose(logfile);
		}
	}
}

void extract_feed_items(xmlNodeSetPtr nodes) {
	xmlNodePtr cur, child;
	xmlChar *textNode;
	int size, i, len;
	int name_set, url_set;
	char *str;
	list_elem item = newItem();
	size = (nodes) ? nodes->nodeNr : 0;

	dbg_printf(P_MSG, "%d items in XML\n", size);
	for(i = 0; i < size; ++i) {
		assert(nodes->nodeTab[i]);
		if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
			cur = nodes->nodeTab[i];
			if(cur->children) {
				child = cur->children;
				url_set = 0;
				name_set = 0;
				while(child) {
					if((strcmp((char*)child->name, "title") == 0) ||
						(strcmp((char*)child->name, "link" ) == 0)) {
						textNode = xmlNodeGetContent(child->children);
						if(!textNode) {
							dbg_printf(P_ERROR, "Child of %s does not seem to be a text node\n", child->name);
							continue;
						} else {
							dbg_printf(P_INFO2, "Found text node '%s'\n", textNode);
							len = strlen((char*)textNode);
							str = malloc(len + 1);
							if(str) {
								dbg_printf(P_INFO2, "allocated %d bytes for 'str'\n", len+1);
								strcpy(str, (char*)textNode);
								if(strcmp((char*)child->name, "title") == 0) {
									item.name = str;
									name_set = 1;
								} else if(strcmp((char*)child->name, "link") == 0) {
									item.url = str;
									url_set = 1;
								}
							} else {
								dbg_printf(P_ERROR, "Error allocating %d bytes for 'str': %s \n", len+1, strerror(errno));
								name_set = 0;
							}
							xmlFree(textNode);
						}
					}
					if(name_set && url_set) {
						break;
					}
					child = child->next;
				}
				if(name_set && url_set) {
					addItem(item, &rss_items);
				}
				child = cur = NULL;
			}
		} else {
			cur = nodes->nodeTab[i];
			dbg_printf(P_ERROR, "Unknown node \"%s\": type %d\n", cur->name, cur->type);
		}
	}
}

int parse_xmldata(const char* buffer, int size, const xmlChar* xpathExpr) {
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
		xmlCleanupParser();
		return -1;
	}

	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
	if(xpathObj == NULL) {
		dbg_printf(P_ERROR, "Error: unable to evaluate XPath expression \"%s\"\n", xpathExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return -1;
	}
	extract_feed_items(xpathObj->nodesetval);
	/* Cleanup */
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	/* Shutdown libxml */
	xmlCleanupParser();
	return 0;
}

char* http_getfile(const char *url, int *plength) {
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
			/* malloc is shaky here because the content might be gzipped -> content_length is smaller than the inflated data */
			content = malloc(MAX_BUFFER);
			if(!content) {
				perror("malloc");
				content = NULL;
			} else {
				dbg_printf(P_INFO, "[http_getfile] allocated %d bytes for 'content'\n", MAX_BUFFER);
				res = xmlNanoHTTPRead(ctxt, content, MAX_BUFFER);
				dbg_printf(P_INFO, "xmlNanoHTTPRead: %d\n", res);
				if(res < MAX_BUFFER) {
					content = realloc(content, res);
					if(!content) {
						dbg_printf(P_ERROR, "Error: realloc() failed: %s\n", strerror(errno));
					} else {
						dbg_printf(P_INFO, "reallocated 'data' to %d byte\n", res);
					}
					if (plength) {
						*plength = res;
					}
				} else if(res == MAX_BUFFER) {
					dbg_printf(P_ERROR, "HTTP response larger than %db! Buffer too small\n", MAX_BUFFER);
					free(content);
					dbg_printf(P_INFO, "[http_getfile] freed 'content'\n");
				} else {
					dbg_printf(P_ERROR, "xmlNanoHTTPRead failed: %d\n", res);
					free(content);
					dbg_printf(P_INFO, "[http_getfile] freed 'content'\n");
				}
			}
		} else {
			dbg_printf(P_ERROR, "Could not determine content length\n");
		}
	} else {
		dbg_printf(P_ERROR, "Uncaught HTTP response: %d\n", xmlNanoHTTPReturnCode(ctxt));
	}
	if(ctxt) {
		xmlNanoHTTPClose(ctxt);
	}
	xmlNanoHTTPCleanup();
	/* xmlMemoryDump(); */
	return content;
}

int parse_config_file(const char *filename) {
	FILE *f;
	char *buf, *p;
	struct stat fs;
	size_t fsize, read_bytes;
	unsigned int len;
	list_elem re = newItem();


	if(stat(filename, &fs) == -1) {
		perror("Unable to determine file size of config file");
		return 1;
	}
	fsize = fs.st_size;
	f = fopen(filename, "r");
	if(f == NULL) {
		perror("fopen");
		return 1;
	}

	buf = malloc(fsize);
	if(!buf) {
		perror("malloc");
		return 1;
	}
	printf("%s\n", buf);
	dbg_printf(P_INFO, "[parse_config_file] allocated %d bytes for 'buf'\n", fsize);
	read_bytes = fread(buf, 1, fsize, f);
	dbg_printf(P_INFO, "[parse_config_file] buf: %s\n---\n", buf);
	fclose(f);
	p = strtok(buf, "\n");
	while (p) {
		len = strlen(p);
		re.name = malloc(len + 1);
		if(re.name) {
			dbg_printf(P_INFO, "[parse_config_file] allocated %d bytes for '%s'\n", len + 1, p);
			strcpy(re.name, p);
			addItem(re, &regex_items);
		} else {
			perror("malloc");
		}
		p = strtok(NULL, "\n");
	}
	free(buf);
	dbg_printf(P_INFO, "[parse_config_file] freed 'buf'\n");
	return 0;
}

void print_list(linked_list list) {
	NODE *cur = list;
	while(cur != NULL) {
		if(cur->elem.name != NULL) {
			dbg_printf(P_INFO, "%s\n", cur->elem.name);
		}
		cur = cur->pNext;
	}
}

void check_for_downloads() {
	int err;
	regex_t preg;
	size_t len;
	char erbuf[100];

	NODE *current_regex = regex_items;
	print_list(regex_items);
	while (current_regex != NULL) {
		NODE *current_rss_item = rss_items;
		dbg_printf(P_INFO2, "Current regex: %s\n", current_regex->elem.name);
		err = regcomp(&preg, current_regex->elem.name, REG_EXTENDED|REG_ICASE);
		if(err) {
			dbg_printf(P_INFO2, "regcomp: Error compiling regular expression: %d\n", err);
			current_regex = current_regex->pNext;
			continue;
		}
		while (current_rss_item != NULL) {
			dbg_printf(P_INFO2, "Current rss_item: %s\n", current_rss_item->elem.name);
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
				/* TODO: need to copy the elem, otherwise we lose name/url */
				addItem(current_rss_item, &bucket);
			}
			current_rss_item = current_rss_item->pNext;
		}
		current_regex = current_regex->pNext;
		regfree(&preg);
	}
}

void add_to_bucket(list_elem item, NODE **b) {
	addItem(item, b);
	if(listCount(*b) > MAX_ITEMS) {
		dbg_printf(P_INFO, "[add_to_bucket] bucket gets too large, deleting head item...\n");
		freeItem(*b);
		deleteHead(b);
	}
}

void freeItem(NODE *item) {
	if(item != NULL) {
		if(item->elem.name) {
			free(item->elem.name);
			dbg_printf(P_INFO, "[freeItem] freed 'elem.name'\n");
		}
		if(item->elem.url) {
			free(item->elem.url);
			dbg_printf(P_INFO, "[freeItem] freed 'elem.url'\n");
		}
	}
}

void cleanup_list(NODE **list) {
	NODE *current = *list;
	dbg_printf(P_INFO, "[cleanup_list] list size before: %d\n", listCount(*list));
	while (current != NULL) {
		freeItem(current);
		current = current->pNext;
	}
	freeList(list);
	dbg_printf(P_INFO, "[cleanup_list] list size after: %d\n", listCount(*list));
}

void do_cleanup(void) {
	cleanup_list(&regex_items);
	cleanup_list(&rss_items);
	cleanup_list(&bucket);
}

void signal_handler(int sig) {
	switch(sig) {
	case SIGTERM:
		dbg_printf(P_MSG,"SIGTERM caught\n");
		shutdown_daemon();
		break;
	}
}

void shutdown_daemon() {
	char time_str[64];
	dbg_printf(P_MSG,"%s: Shutting down daemon\n", getlogtime_str(time_str, sizeof(time_str)));
	do_cleanup();
	exit(0);
}

int daemonize() {
#ifndef NODAEMON
	int i, lfp;
	int fd;
	char str[10];
	if(getppid() == 1)
		return -1; /* already a daemon */
	switch(fork()) {
		case 0:
         break;
      case -1:
         fprintf(stderr, "Error daemonizing (fork)! %d - %s\n", errno, strerror(errno));
         return -1;
      default:
         _exit(0);
	}
	if( setsid() < 0 ) {
      fprintf( stderr, "Error daemonizing (setsid)! %d - %s\n", errno, strerror(errno) );
      return -1;
	}
	/* first instance continues */
	lfp = open(LOCK_FILE,O_RDWR|O_CREAT,0640);
	if (lfp < 0) {
		fprintf(stderr, "Failed to open lockfile (%s). Aborting...\n", strerror(errno));
		exit(1); /* can not open */
	}
	if (lockf(lfp,F_TLOCK,0) < 0) {
		fprintf(stderr, "Cannot lock pid file. Another instance running? Aborting...\n");
		exit(0); /* can not lock */
	}
	sprintf(str, "%d\n", getpid());
	write(lfp, str, strlen(str)); /* record pid to lockfile */
	for(i = getdtablesize(); i >= 0; --i) {
		close(i); /* close all descriptors */
	}
	fd = open("/dev/null", O_RDONLY);
	if( fd != 0 ) {
		dup2( fd,  0 );
		close( fd );
	}
	fd = open("/dev/null", O_WRONLY);
	if( fd != 1 ) {
		dup2( fd, 1 );
		close( fd );
	}
	fd = open("/dev/null", O_WRONLY);
	if( fd != 2 ) {
		dup2( fd, 2 );
		close( fd );
	}
#endif
	umask(027); /* set newly created file permissions */
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTERM,signal_handler); /* catch kill signal */
	return 0;
}


char* getlogtime_str( char * buf, int buflen ) {
	char tmp[64];
	time_t now;
	struct tm now_tm;
	struct timeval tv;

	now = time( NULL );
	gettimeofday( &tv, NULL );

	localtime_r( &now, &now_tm );
	strftime( tmp, sizeof(tmp), "%y/%m/%d %H:%M:%S", &now_tm );
	/*milliseconds = (int)(tv.tv_usec / 1000);*/
	snprintf( buf, buflen, "%s", tmp);

	return buf;
}

int main(int argc, char **argv) {
	char *data = NULL;
	int data_size = 0;
#ifndef NODAEMON
	FILE *fp;
	static int interval = 10;
#else
	static int interval = 1;
#endif
	const char *feed_url = "http://tvrss.net/feed/eztv/";
	char time_str[64];

	LIBXML_TEST_VERSION
	/* clear log file */
#ifndef NODAEMON
	fp = fopen(LOG_FILE, "w");
	if(fp == NULL) {
		fprintf(stderr, "Warning: Unable to open '"LOCK_FILE"'! %d - %s\n", errno, strerror(errno) );
	}
	fclose(fp);
#endif
	dbg_printf( P_MSG, "%s: Daemon started\n", getlogtime_str(time_str, sizeof(time_str)));
	if(parse_config_file("test.conf") != 0) {
		fprintf(stderr, "Error parsing config file: %s\n", strerror(errno));
		shutdown_daemon();
		exit(1);
	}
	dbg_printf(P_MSG, "Read %d regular expressions from config file\n", listCount(regex_items));
 	if(daemonize() != 0) {
		fprintf(stderr, "Error: Daemonize failed. Aborting...\n");
		shutdown_daemon();
		exit(1);
	}

	while(1) {
		dbg_printf( P_MSG, "------ %s: Checking for new episodes ------\n", getlogtime_str(time_str, sizeof(time_str)));
		data = http_getfile(feed_url, &data_size);
		if(data) {
			dbg_printf(P_INFO, "XML size: %d\n", data_size);
			if(data_size > 0) {
				parse_xmldata(data, data_size, (xmlChar*)"//item");
			}
			free(data);
			dbg_printf(P_INFO, "[main] freed 'data'\n");
			check_for_downloads();
		}
		cleanup_list(&rss_items);
 		sleep(interval * 10);
	}
	return 0;
}
