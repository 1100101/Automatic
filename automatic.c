#define LOCK_FILE  "/ffp/tmp/automatic.pid"
#define MEMWATCH 1
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
#include <sys/param.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "memwatch.h"
#include "list.h"
#include "version.h"
#include "web.h"
#include "output.h"

#define MAX_BUFFER 128 * 1024   /* max. file size for xml/torrent = 128k */
#define BUF_SIZE 4096
#define MAX_ITEMS 10
#define TIME_STR_SIZE 20

void shutdown_daemon(void);
void cleanup_list(NODE **list);
void getlogtime_str( char * buf );
void freeItem(NODE *item);

static linked_list rss_items = NULL;
static linked_list regex_items = NULL;
static linked_list bucket = NULL;

static regex_t *content_disp_preg;

rss_item newItem() {
	rss_item i;
	i.name = NULL;
	i.url = NULL;
	return i;
}

int verbose = P_MSG;
int gl_debug   = 0;
char logfile[MAXPATHLEN]  = "/ffp/tmp/automatic.log";
static char configfile[MAXPATHLEN]  = "/ffp/etc/automatic.conf";


void usage() {
   printf(
  "usage: automatic -u url [-fh] [-v level] [-i minutes] [-l file]\n"
  "\n"
  "Automatic %s\n"
  "\n"
  "  -u --url <url>            Feed address\n"
  "  -f --nodeamon             Run in the foreground and log to stderr\n"
  "  -h --help                 Display this message\n"
  "  -v --verbose <level>      Set output level to <level> (default=1)\n"
  "  -i --interval <min>       Set check interval for RSS feed to <min>\n"
  "  -l --logfile <path>       Place the log file at <path>\n"
  "  -c --configfile <path>    Path to configuration file\n"
  "\n", LONG_VERSION_STRING );
    exit( 0 );
}

void readargs( int argc, char ** argv, int * nofork, int * interval, char ** l_file, char ** c_file, char ** url ) {
    char optstr[] = "fhv:i:l:c:u:";
    struct option longopts[] =
    {
        { "verbose",            required_argument, NULL, 'v' },
        { "nodaemon",           no_argument,       NULL, 'f' },
        { "help",               no_argument,       NULL, 'h' },
		  { "interval",           required_argument, NULL, 'i' },
		  { "logfile",            required_argument, NULL, 'l' },
		  { "configfile",         required_argument, NULL, 'c' },
		  { "url",                required_argument, NULL, 'u' },
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
            case 'c':
                *c_file = optarg;
                break;
            case 'u':
                *url = optarg;
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
	rss_item item = newItem();
	size = (nodes) ? nodes->nodeNr : 0;

	dbg_printf(P_INFO2, "%d items in XML\n", size);
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
							dbg_printf(P_ERROR, "Child of '%s' does not seem to be a text node\n", child->name);
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

int parse_config_file(const char *filename) {
	FILE *f;
	char *buf, *p;
	struct stat fs;
	size_t fsize, read_bytes;
	unsigned int len;
	rss_item re;


	if(stat(filename, &fs) == -1) {
		return 1;
	}
	fsize = fs.st_size;
	f = fopen(filename, "r");
	if(f == NULL) {
		return 1;
	}

	buf = malloc(fsize);
	if(!buf) {
		return 1;
	}
	dbg_printf(P_INFO2, "[parse_config_file] allocated %d bytes for 'buf'\n", fsize);
	read_bytes = fread(buf, 1, fsize, f);
	dbg_printf(P_INFO2, "[parse_config_file] buf:\n%s\n---\n", buf);
	fclose(f);
	re = newItem();
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
		if(cur->item.name != NULL) {
			dbg_printf(P_INFO, "\t%s\n", cur->item.name);
		}
		cur = cur->pNext;
	}
}

int has_been_downloaded(char *url) {
	return hasURL(url, bucket);
}

int add_to_bucket(rss_item elem, NODE **b) {
	rss_item b_elem = newItem();
	/* copy element content, as the original is going to be freed later on */
	int integrity_check = 1;
	if(elem.name != NULL) {
		dbg_printf(P_INFO2, "[add_to_bucket] elem.name: %s\n", elem.name);
		b_elem.name = malloc(strlen(elem.name) + 1);
		if(b_elem.name) {
			dbg_printf(P_INFO2, "[add_to_bucket] allocated %d bytes for '%s'\n", strlen(elem.name) + 1, elem.name);
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
			dbg_printf(P_INFO2, "[add_to_bucket] allocated %d bytes for '%s'\n", strlen(elem.url) + 1, elem.url);
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


int call_transmission(const char *filename) {
	char buf[MAXPATHLEN];
	int ret;

	if(filename && strlen(filename) > 1) {
		sprintf(buf, "transmission-remote -a \"%s\"", filename);
		dbg_printf(P_INFO, "[call_transmission] calling transmission-remote...");
		ret = system(buf);
		if(ret == -1) {
			dbg_printf(P_ERROR, "\n[call_transmission] Error calling sytem(): %s\n", strerror(errno));
			return -1;
		} else {
			dbg_printf(P_INFO, "Success\n");
			return 0;
		}
	} else {
		dbg_printf(P_ERROR, "[call_transmission] Error: no filename provided (%p)\n", (void*)filename);
		return -1;
	}
}

int is_torrent(const char *str) {
	if(strstr(str, ".torrent"))
		return 1;
	else
		return 0;
}

void get_filename(WebData *data, char *file_name) {
	char *p, *tmp, fname[MAXPATHLEN], buf1[MAXPATHLEN], buf2[MAXPATHLEN], erbuf[100];
	int len,err, filename_found = 0;
	regmatch_t pmatch[2];

	assert(data);

	/* Case 1: Use Header field "Content-Disposition" (if available) to get correct filename */
	tmp = malloc(data->header->size);
	dbg_printf(P_INFO2, "[get_filename] allocated %d bytes for tmp\n", data->header->size);
	memcpy(tmp, data->header->data, data->header->size);
	p = strtok(tmp, "\r\n");
	if(!content_disp_preg) {
		dbg_printf(P_ERROR, "[get_filename] Error: content_disp_preg not initialized\n");
	} else {
		while (p) {
			if(filename_found == 0) {
				strcpy(buf1, p);
				dbg_printf(P_INFO2, "[get_filename] Current header line: %s\n", buf1);
				err = regexec(content_disp_preg, buf1, 2, pmatch, 0);
				if(!err) {			/* regex matches */
					len = pmatch[1].rm_eo - pmatch[1].rm_so;
					strncpy(buf2, buf1+pmatch[1].rm_so, len);
					buf2[len] = '\0';
					dbg_printf(P_INFO, "[get_filename] Found filename: %s\n", buf2);
					filename_found = 1;
				} else if(err != REG_NOMATCH && err != 0){
					len = regerror(err, content_disp_preg, erbuf, sizeof(erbuf));
					dbg_printf(P_ERROR, "[get_filename] regexec error: %s\n", erbuf);
				} else {
					if(!strncmp(buf1, "Content-Disposition", 19)) {
						dbg_printf(P_ERROR, "[get_filename] Unknown pattern: %s\n", buf1);
					}
				}
			}
			p = strtok(NULL, "\r\n");
		}
	}
	if(tmp)
		free(tmp);

	/* Case 2: Parse URL for filename */
	if(filename_found == 0) {
		strcpy(buf1, data->url);
		p = strtok(buf1, "/");
		while (p) {
			len = strlen(p);
			if(len < MAXPATHLEN)
				strcpy(buf2, p);
			p = strtok(NULL, "/");
		}
	}
	strcpy(fname, "/ffp/tmp/");
	strcat(fname, buf2);
	if(!is_torrent(buf2)) {
		strcat(fname, ".torrent");
	}
	strcpy(file_name, fname);
}

void download_torrent(NODE *item) {
	char fname[MAXPATHLEN];
	int torrent;
	int res;
	WebData *wdata;

	dbg_printf(P_MSG, "Found new download: %s (%s)\n", item->item.name, item->item.url);
	wdata = getHTTPData(item->item.url);
	if(wdata && wdata->response) {
		get_filename(wdata, fname);
		torrent = open(fname,O_RDWR|O_CREAT, 00444);
		if(torrent == -1) {
			dbg_printf(P_ERROR, "Error opening file for writing: %s\n", strerror(errno));
		} else {
			res = write(torrent, wdata->response->data, wdata->response->size);
			if(res != wdata->response->size) {
				dbg_printf(P_ERROR, "[download_torrent] Error writing torrent file: %s\n", strerror(errno));
			} else {
				dbg_printf(P_INFO, "Saved torrent file '%s'\n", fname);
			}
			fchmod(torrent, 00444);
			close(torrent);
			if(call_transmission(fname) == -1) {
				dbg_printf(P_ERROR, "[download_torrent] error adding torrent '%s' to Transmission\n");
			}
			/* unlink(fname); */
		}
	} else {
		dbg_printf(P_ERROR, "Error downloading torrent file (wdata: %p, wdata->response: %p\n", (void*)wdata, (void*)(wdata->response));
	}
	WebData_free(wdata);
	if(!add_to_bucket(item->item, &bucket) == 1) {
		dbg_printf(P_ERROR, "Error: Unable to add matched download to bucket list: %s\n", item->item.name);
	}
}

void check_for_downloads() {
	int err;
	regex_t preg;
	char erbuf[100];
	NODE *current_regex = regex_items;
	while (current_regex != NULL) {
		NODE *current_rss_item = rss_items;
		dbg_printf(P_INFO2, "Current regex: %s\n", current_regex->item.name);
		err = regcomp(&preg, current_regex->item.name, REG_EXTENDED|REG_ICASE);
		if(err) {
			regerror(err, &preg, erbuf, sizeof(erbuf));
			dbg_printf(P_INFO2, "regcomp: Error compiling regular expression: %s\n", erbuf);
			current_regex = current_regex->pNext;
			continue;
		}
		while (current_rss_item != NULL) {
			dbg_printf(P_INFO2, "Current rss_item: %s\n", current_rss_item->item.name);
			err = regexec(&preg, current_rss_item->item.name, 0, NULL, 0);
			if(!err && !has_been_downloaded(current_rss_item->item.url)) {			/* regex matches and it hasn't been downloaded before */
				download_torrent(current_rss_item);
			} else if(err != REG_NOMATCH && err != 0){
				regerror(err, &preg, erbuf, sizeof(erbuf));
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
		if(item->item.name) {
			free(item->item.name);
			dbg_printf(P_INFO2, "[freeItem] freed 'elem.name'\n");
		}
		if(item->item.url) {
			free(item->item.url);
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
	if(content_disp_preg) {
		regfree(content_disp_preg);
		free(content_disp_preg);
	}
}

void shutdown_daemon() {
	char time_str[TIME_STR_SIZE];
	getlogtime_str(time_str);
	dbg_printf(P_MSG,"%s: Shutting down daemon\n", time_str);
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

void signal_handler(int sig) {
	switch(sig) {
		case SIGINT:
		case SIGTERM: {
			dbg_printf(P_INFO2,"SIGTERM/SIGINT caught\n");
			shutdown_daemon();
			break;
		}
	}
}

void setup_signals(void) {
	signal(SIGCHLD, SIG_IGN); /* ignore child */
	signal(SIGTSTP, SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTERM, signal_handler); /* catch kill signal */
	signal(SIGINT , signal_handler); /* catch kill signal */
}

void getlogtime_str(char *buf) {
	char tmp[TIME_STR_SIZE];
	time_t now;
	struct tm now_tm;
	struct timeval tv;

	now = time( NULL );
	gettimeofday( &tv, NULL );

	localtime_r( &now, &now_tm );
	strftime( tmp, sizeof(tmp), "%y/%m/%d %H:%M:%S", &now_tm );
	snprintf( buf, strlen(tmp) + 1, "%s", tmp);
}


/* the regular expression for the Content-Disposition HTTP header entry never changes, so we
   initialize it once on startup and reuse it */
void init_cd_preg() {
	int err;
	char erbuf[100];
	const char* fname_regex = "Content-Disposition: inline; filename=\"(.+)\"$";

	assert(content_disp_preg == NULL);

	content_disp_preg = malloc(sizeof(regex_t));
	dbg_printf(P_INFO2, "[init_cd_preg] allocated %d bytes for content_disp_preg\n", sizeof(regex_t));
	err = regcomp(content_disp_preg, fname_regex, REG_EXTENDED|REG_ICASE);
	/* a failure of regcomp won't be critical. the app will fall back and extract a filename from the URL */
	if(err) {
		regerror(err, content_disp_preg, erbuf, sizeof(erbuf));
		dbg_printf(P_ERROR, "[init_cd_preg] regcomp: Error compiling regular expression: %s\n", erbuf);
	}
}

int main(int argc, char **argv) {
	FILE *fp;
	int nofork = 0;
	int interval = 10;
	char *log_file = NULL, *conf_file = NULL;
	/* const char *feed_url = "http://tvrss.net/feed/eztv/"; */
	char *feed_url = NULL;
	char time_str[TIME_STR_SIZE];
	char erbuf[100];
	WebData *wdata;

	LIBXML_TEST_VERSION

	readargs(argc, argv, &nofork, &interval, &log_file, &conf_file, &feed_url);

	if(feed_url == NULL)  {
		fprintf(stderr, "\nNo feed URL specified!\n\n");
		usage();
		shutdown_daemon();
	}
	if(log_file != NULL)
		strcpy(logfile, log_file);
	if(conf_file != NULL)
		strcpy(configfile, conf_file);

	/* clear logfile */
	fp = fopen(logfile, "w");
	if(fp == NULL) {
		fprintf(stderr, "[main] Failed to open logfile '%s': %s\n", logfile, strerror(errno));
		exit(1);
	} else {
		fclose(fp);
	}
	dbg_printf(P_INFO, "check interval: %d min\n", interval);
	dbg_printf(P_INFO, "verbose level: %d\n", verbose);
	dbg_printf(P_INFO, "log file: %s\n", nofork ? "stderr" : logfile);
	dbg_printf(P_INFO, "config path: %s\n", configfile);
	dbg_printf(P_INFO, "Feed URL: %s\n", feed_url);

	if(parse_config_file(configfile) != 0) {
		if(errno == ENOENT) {
			snprintf(erbuf, sizeof(erbuf), "Cannot find file '%s'", configfile);
		} else {
			snprintf(erbuf, sizeof(erbuf), "%s", strerror(errno));
		}
		fprintf(stderr, "Error parsing config file: %s\n", erbuf);
/* 		shutdown_daemon(); */
		exit(1);
	}
	dbg_printf(P_MSG, "Read %d patterns from config file\n", listCount(regex_items));
	print_list(regex_items);
 	if(!nofork) {
		if(daemonize() != 0) {
			fprintf(stderr, "Error: Daemonize failed. Aborting...\n");
			shutdown_daemon();
			exit(1);
		}
		getlogtime_str(time_str);
		dbg_printf( P_MSG, "%s: Daemon started\n", time_str);
	}
	setup_signals();
	init_cd_preg();

	while(1) {
		getlogtime_str(time_str);
		dbg_printf( P_MSG, "------ %s: Checking for new episodes ------\n", time_str);
		wdata = getHTTPData(feed_url);
		if(wdata && wdata->response) {
			if(wdata->response->size > 0) {
				dbg_printf(P_INFO2, "wdata->response->data: %d\n", strlen(wdata->response->data));
				parse_xmldata(wdata->response->data, wdata->response->size, (xmlChar*)"//item");
			}
			WebData_free(wdata);
			dbg_printf(P_INFO2, "[main] freed 'data'\n");
			check_for_downloads();
		}
		cleanup_list(&rss_items);
 		sleep(interval * 60);
	}
	return 0;
}
