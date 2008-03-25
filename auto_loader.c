#define NODAEMON

#define LOCK_FILE	"/tmp/auto_loader.pid"

#define LONG_VERSION_STRING "0.1.0"

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
#include <getopt.h>
#include <sys/types.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>

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
static int gl_debug   = 0;
static char logfile[MAXPATHLEN]  = "/tmp/auto_loader.log";

void dbg_printf(debug_type type, const char *format, ...) {
	va_list va;
   char tmp[MSGSIZE_MAX];
	int print_msg = 0;
	FILE *fp;
	switch(type) {
		case P_ERROR:
		case P_MSG: {
			print_msg = 1;
			break;
		}
		case P_INFO: {
			if(verbose <= 2)
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
		if(gl_debug == 0) {
			fp = fopen(logfile,"a");
			if(fp) {
				fprintf(fp,"%s", tmp);
				fclose(fp);
			}
		} else {
			fprintf(stderr, "%s", tmp);
		}
	}
}


void usage() {
   printf(
  "usage: auto_loader [-fh] [-v level] [-i minutes] [-l file]\n"
  "\n"
  "auto_loader %s\n"
  "\n"
  "  -f --nodeamon             Run in the foreground and log to stderr\n"
  "  -h --help                 Display this message\n"
  "  -v --verbose <level>      Set output level to <level> (default=1)\n"
  "  -i --interval <min>       Set check interval for RSS feed to <min>\n"
  "  -l --logfile <path>       Place the log file at <path>\n"
  "\n", LONG_VERSION_STRING );
    exit( 0 );
}

void readargs( int argc, char ** argv, int * nofork, int * interval, char ** l_file ) {
    char optstr[] = "fhv:i:l:";
    struct option longopts[] =
    {
        { "verbose",            required_argument, NULL, 'v' },
        { "nodaemon",           no_argument,       NULL, 'f' },
        { "help",               no_argument,       NULL, 'h' },
		  { "interval",           required_argument, NULL, 'i' },
		  { "logfile",            required_argument, NULL, 'l' },
        { NULL, 0, NULL, 0 }
    };
    int opt;

    *nofork    = 0;
    *l_file   = NULL;

    while( 0 <= ( opt = getopt_long( argc, argv, optstr, longopts, NULL ) ) )
    {
        switch( opt )
        {
            case 'v':
                verbose = atoi(optarg);
                break;
            case 'f':
                *nofork = 1;
					 gl_debug = 1;
                break;
            case 'i':
                *interval = atoi(optarg);
                break;
            case 'l':
                *l_file = optarg;
                break;
            default:
                usage();
                break;
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
		content_length = xmlNanoHTTPContentLength(ctxt);
		if(content_length > 0) {
			/* malloc is shaky here because the content might be gzipped -> content_length is smaller than the inflated data */
			content = malloc(MAX_BUFFER);
			if(!content) {
				perror("malloc");
				content = NULL;
			} else {
				dbg_printf(P_INFO2, "[http_getfile] allocated %d bytes for 'content'\n", MAX_BUFFER);
				res = xmlNanoHTTPRead(ctxt, content, MAX_BUFFER);
				dbg_printf(P_INFO, "Downloaded %d bytes from server\n", res);
				if(res < MAX_BUFFER) {
					content = realloc(content, res);
					if(!content) {
						dbg_printf(P_ERROR, "Error: realloc() failed: %s\n", strerror(errno));
					} else {
						dbg_printf(P_INFO2, "reallocated 'data' to %d byte\n", res);
					}
					if (plength) {
						*plength = res;
					}
				} else if(res == MAX_BUFFER) {
					dbg_printf(P_ERROR, "HTTP response larger than %db! Buffer too small\n", MAX_BUFFER);
					free(content);
					dbg_printf(P_INFO2, "[http_getfile] freed 'content'\n");
				} else {
					dbg_printf(P_ERROR, "xmlNanoHTTPRead failed: %d\n", res);
					free(content);
					dbg_printf(P_INFO2, "[http_getfile] freed 'content'\n");
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
	dbg_printf(P_INFO2, "[parse_config_file] allocated %d bytes for 'buf'\n", fsize);
	read_bytes = fread(buf, 1, fsize, f);
	dbg_printf(P_INFO2, "[parse_config_file] buf:\n%s\n---\n", buf);
	fclose(f);
	p = strtok(buf, "\n");
	while (p) {
		len = strlen(p);
		re.name = malloc(len + 1);
		if(re.name) {
			dbg_printf(P_INFO2, "[parse_config_file] allocated %d bytes for '%s'\n", len + 1, p);
			strcpy(re.name, p);
			addItem(re, &regex_items);
		} else {
			perror("malloc");
		}
		p = strtok(NULL, "\n");
	}
	free(buf);
	dbg_printf(P_INFO2, "[parse_config_file] freed 'buf'\n");
	return 0;
}

void print_list(linked_list list) {
	NODE *cur = list;
	while(cur != NULL) {
		if(cur->elem.name != NULL) {
			dbg_printf(P_INFO, "\t%s\n", cur->elem.name);
		}
		cur = cur->pNext;
	}
}

int has_been_downloaded(char *url) {
	return hasURL(url, bucket);
}

int add_to_bucket(list_elem elem, NODE **b) {
	list_elem b_elem = newItem();
	/* copy element content, as the original is going to be freed later on */
	int integrity_check = 1;
	if(elem.name != NULL) {
		dbg_printf(P_INFO2, "[add_to_bucket] elem.name: %s\n", elem.name);
		b_elem.name = malloc(strlen(elem.name) + 1);
		if(b_elem.name) {
			strcpy(b_elem.name, elem.name);
		} else {
			dbg_printf(P_ERROR, "[add_to_bucket] malloc failed: %s\n", strerror(errno));
			integrity_check &= 0;
		}
	}
	if(elem.url != NULL) {
		dbg_printf(P_INFO2, "[add_to_bucket] elem.url: %s\n", elem.url);
		b_elem.url = malloc(strlen(elem.url) + 1);
		if(b_elem.url) {
			strcpy(b_elem.url, elem.url);
		} else {
			dbg_printf(P_ERROR, "[add_to_bucket] malloc failed: %s\n", strerror(errno));
			integrity_check &= 0;
		}
	}
	if(integrity_check) {
		addItem(b_elem, b);
		if(listCount(*b) > MAX_ITEMS) {
			dbg_printf(P_INFO, "[add_to_bucket] bucket gets too large, deleting head item...\n");
			freeItem(*b);
			deleteHead(b);
		}
	}
	return integrity_check;
}

void check_for_downloads() {
	int err;
	regex_t preg;
	size_t len;
	char erbuf[100];

	NODE *current_regex = regex_items;
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
			if(!err && !has_been_downloaded(current_rss_item->elem.url)) {			/* regex matches and it hasn't been downloaded before */
				if(add_to_bucket(current_rss_item->elem, &bucket) == 1) {
					dbg_printf(P_MSG, "Found new download: %s\n", current_rss_item->elem.name);
					if(xmlNanoHTTPFetch(current_rss_item->elem.url, "/tmp/tmp.torrent", NULL) != 0) {
						dbg_printf(P_ERROR, "Error downloading torrent file\n", current_rss_item->elem.name);
					}
				} else {
					dbg_printf(P_ERROR, "Error: Unable to add matched download to bucket list: %s\n", current_rss_item->elem.name);
				}
			} else if(err != REG_NOMATCH && err != 0){
				len = regerror(err, &preg, erbuf, sizeof(erbuf));
				dbg_printf(P_ERROR, "[check_for_downloads] regexec error: %s\n", erbuf);
			}
			current_rss_item = current_rss_item->pNext;
		}
		regfree(&preg);
		current_regex = current_regex->pNext;
	}
}

void freeItem(NODE *item) {
	if(item != NULL) {
		if(item->elem.name) {
			free(item->elem.name);
			dbg_printf(P_INFO2, "[freeItem] freed 'elem.name'\n");
		}
		if(item->elem.url) {
			free(item->elem.url);
			dbg_printf(P_INFO2, "[freeItem] freed 'elem.url'\n");
		}
	}
}

void cleanup_list(NODE **list) {
	NODE *current = *list;
	dbg_printf(P_INFO2, "[cleanup_list] list size before: %d\n", listCount(*list));
	while (current != NULL) {
		freeItem(current);
		current = current->pNext;
	}
	freeList(list);
	dbg_printf(P_INFO2, "[cleanup_list] list size after: %d\n", listCount(*list));
}

void do_cleanup(void) {
	cleanup_list(&regex_items);
	cleanup_list(&rss_items);
	cleanup_list(&bucket);
}

void signal_handler(int sig) {
	switch(sig) {
		case SIGINT:
		case SIGTERM: {
			dbg_printf(P_MSG,"SIGTERM caught\n");
			shutdown_daemon();
			break;
		}
	}
}

void shutdown_daemon() {
	char time_str[64];
	dbg_printf(P_MSG,"%s: Shutting down daemon\n", getlogtime_str(time_str, sizeof(time_str)));
	do_cleanup();
	exit(0);
}

int daemonize() {
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
	umask(027); /* set newly created file permissions */
	return 0;
}

void setup_signals(void) {
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTERM,signal_handler); /* catch kill signal */
	signal(SIGINT,signal_handler); /* catch kill signal */
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
	FILE *fp;
	int nofork = 0;
	int interval = 10;
	const char *feed_url = "http://tvrss.net/feed/eztv/";
	char *log_file;
	char time_str[64];

	LIBXML_TEST_VERSION
	readargs(argc, argv, &nofork, &interval, &log_file );

	if(!nofork && (log_file != NULL)) {
		fp = fopen(log_file, "w");
		if(fp == NULL) {
			fprintf(stderr, "Failed to open logfile: %s\n", log_file);
			fprintf(stderr, "Fallback to default '%s'\n", logfile);
		} else {
			fclose(fp);
			strncpy(logfile, log_file, sizeof(logfile));
			logfile[MAXPATHLEN-1] = '\0';
		}
	}

	if(!nofork) {
		/* clear log file */
		fp = fopen(logfile, "w");
		if(fp == NULL) {
			fprintf(stderr, "Failed to open logfile: %s\n", logfile);
			exit(1);
		} else {
			fclose(fp);
		}
	}
	dbg_printf( P_MSG, "%s: Daemon started\n", getlogtime_str(time_str, sizeof(time_str)));
	dbg_printf(P_INFO, "check interval: %d min\n", interval);
	dbg_printf(P_INFO, "verbose level: %d\n", verbose);
	dbg_printf(P_INFO, "log file: %s\n", nofork ? "stderr" : logfile);
	if(parse_config_file("test.conf") != 0) {
		fprintf(stderr, "Error parsing config file: %s\n", strerror(errno));
		shutdown_daemon();
		exit(1);
	}
	dbg_printf(P_MSG, "Read %d regular expressions from config file\n", listCount(regex_items));
	print_list(regex_items);
 	if(!nofork) {
		if(daemonize() != 0) {
			fprintf(stderr, "Error: Daemonize failed. Aborting...\n");
			shutdown_daemon();
			exit(1);
		}
	}
	setup_signals();
	while(1) {
		dbg_printf( P_MSG, "------ %s: Checking for new episodes ------\n", getlogtime_str(time_str, sizeof(time_str)));
		data = http_getfile(feed_url, &data_size);
		if(data) {
			if(data_size > 0) {
				parse_xmldata(data, data_size, (xmlChar*)"//item");
			}
			free(data);
			dbg_printf(P_INFO2, "[main] freed 'data'\n");
			check_for_downloads();
		}
		cleanup_list(&rss_items);
 		sleep(interval * 60);
	}
	return 0;
}
