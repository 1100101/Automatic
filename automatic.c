
#define LOCK_FILE  "/ffp/tmp/automatic.pid"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
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
#include <sys/time.h>
#include <sys/wait.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#ifdef MEMWATCH
	#include "memwatch.h"
#endif

#include "list.h"
#include "version.h"
#include "web.h"
#include "output.h"
#include "config_parser.h"
#include "state.h"

#define MAX_BUFFER 128 * 1024   /* max. file size for xml/torrent = 128k */
#define BUF_SIZE 4096
#define TIME_STR_SIZE 20

void shutdown_daemon(void);
void cleanup_list(NODE **list);
void getlogtime_str( char * buf );

static linked_list rss_items = NULL;
linked_list bucket = NULL;
linked_list regex_patterns = NULL;



uint8_t verbose = P_MSG;
uint8_t nofork = 0;
uint8_t max_bucket_items = 10;
uint8_t bucket_changed = 0;
char log_file[MAXPATHLEN + 1] = "/ffp/tmp/automatic.log";
char state_file[MAXPATHLEN + 1] = "/mnt/USB/.automatic/state.txt";
char config_path[MAXPATHLEN + 1] = "/mnt/HD_a2/.transmission";
char *feed_url;
char *configfile = NULL;

uint8_t check_interval = 10;
uint8_t use_transmission = 1;

void usage() {
   printf(
  "usage: automatic [-fh] [-v level] [-c file]\n"
  "\n"
  "Automatic %s\n"
  "\n"
  "  -f --nodeamon             Run in the foreground and log to stderr\n"
  "  -h --help                 Display this message\n"
  "  -v --verbose <level>      Set output level to <level> (default=1)\n"
  "  -c --configfile <path>    Path to configuration file\n"
  "\n", LONG_VERSION_STRING );
    exit( 0 );
}

void readargs( int argc, char ** argv, char **c_file) {
	char optstr[] = "fhv:c:";
   struct option longopts[] = {
		{ "verbose",            required_argument, NULL, 'v' },
		{ "nodaemon",           no_argument,       NULL, 'f' },
		{ "help",               no_argument,       NULL, 'h' },
		{ "configfile",         required_argument, NULL, 'c' },
		{ NULL, 0, NULL, 0 }
	};
   int opt;

   while( 0 <= ( opt = getopt_long( argc, argv, optstr, longopts, NULL ) ) ) {
		switch( opt ) {
			case 'v':
				verbose = atoi(optarg);
				break;
			case 'f':
				nofork = 1;
				break;
			case 'c':
				*c_file = optarg;
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
	uint8_t name_set, url_set;
	char *str;
	rss_item item = newRSSItem();
	size = (nodes) ? nodes->nodeNr : 0;

	if(size > 0)
		max_bucket_items = size;

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
							len = strlen((char*)textNode) + 1;
							str = malloc(len);
							if(str) {
								dbg_printf(P_INFO2, "allocated %d bytes for 'str'\n", len);
								strncpy(str, (char*)textNode, len);
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
					addRSSItem(item, &rss_items);
				}
				child = cur = NULL;
			}
		} else {
			cur = nodes->nodeTab[i];
			dbg_printf(P_ERROR, "Unknown node \"%s\": type %d\n", cur->name, cur->type);
		}
	}
}

int parse_xmldata(const char* buffer, int size) {
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	const xmlChar* xpathExpr = (xmlChar*)"//item";

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


uint8_t has_been_downloaded(char *url) {
	uint8_t res;

	res = hasURL(url, bucket);
	if(res)
		dbg_printf(P_INFO2, "Torrent has already been downloaded\n");
	return res;
}


int call_transmission(const char *filename) {
	char buf[MAXPATHLEN];
	int8_t status;

	if(filename && strlen(filename) > 1) {
		sprintf(buf, "transmission-remote -g \"%s\" -a \"%s\"", config_path, filename);
		dbg_printf(P_INFO, "[call_transmission] calling transmission-remote...\n");
		status = system(buf);
		dbg_printf(P_INFO, "[call_transmission] WEXITSTATUS(status): %d\n", WEXITSTATUS(status));
		if(status == -1) {
			dbg_printf(P_ERROR, "\n[call_transmission] Error calling sytem(): %s\n", strerror(errno));
			return -1;
		} else {
			dbg_printf(P_INFO, "[call_transmission] Success\n");
			return 0;
		}
	} else {
		dbg_printf(P_ERROR, "[call_transmission] Error: invalid filename (addr: %p)\n", (void*)filename);
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
	char *p, tmp[MAXPATHLEN], fname[MAXPATHLEN], buf[MAXPATHLEN];
	int len;

#ifdef DEBUG
	assert(data);
#endif

	if(data->content_filename) {
		strncpy(buf, data->content_filename, strlen(data->content_filename) + 1);
	} else {
		strcpy(tmp, data->url);
		p = strtok(tmp, "/");
		while (p) {
			len = strlen(p);
			if(len < MAXPATHLEN)
				strcpy(buf, p);
			p = strtok(NULL, "/");
		}
	}
	strcpy(fname, "/ffp/tmp/");
	strcat(fname, buf);
	if(!is_torrent(buf)) {
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
			if(use_transmission && call_transmission(fname) == -1) {
				dbg_printf(P_ERROR, "[download_torrent] error adding torrent '%s' to Transmission\n");
				sleep(1);
				unlink(fname);
			}
		}
	} else {
		dbg_printf(P_ERROR, "Error downloading torrent file (wdata: %p, wdata->response: %p\n", (void*)wdata, (void*)(wdata->response));
	}
	WebData_free(wdata);
	if(add_to_bucket(item->item, &bucket, 1) == 1) {
		bucket_changed = 1;
	} else {
		dbg_printf(P_ERROR, "Error: Unable to add matched download to bucket list: %s\n", item->item.name);
	}
}

void check_for_downloads() {
	int err;
	regex_t preg;
	char erbuf[100];
	NODE *current_regex = regex_patterns;
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

void do_cleanup(void) {
	cleanup_list(&regex_patterns);
	cleanup_list(&rss_items);
	cleanup_list(&bucket);
	cd_preg_free();

	if(feed_url)
		free(feed_url);
	if(configfile)
		free(configfile);
}

void shutdown_daemon() {
	char time_str[TIME_STR_SIZE];
	getlogtime_str(time_str);
	dbg_printf(P_MSG,"%s: Shutting down daemon\n", time_str);
	save_state(bucket);
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

int main(int argc, char **argv) {
	FILE *fp;

	const char *default_config = "/ffp/etc/automatic.conf";
	char time_str[TIME_STR_SIZE];
	char erbuf[100];
	WebData *wdata;

	LIBXML_TEST_VERSION
	readargs(argc, argv, &configfile);

	if(!configfile) {
		configfile = malloc(strlen(default_config) + 1);
		strncpy(configfile, default_config, strlen(default_config) + 1);
	}

	if(parse_config_file(configfile) != 0) {
		if(errno == ENOENT) {
			snprintf(erbuf, sizeof(erbuf), "Cannot find file '%s'", configfile);
		} else {
			snprintf(erbuf, sizeof(erbuf), "Unknown error");
		}
		fprintf(stderr, "Error parsing config file: %s\n", erbuf);
		exit(1);
	}

	if(!feed_url || strlen(feed_url) < 1)  {
		fprintf(stderr, "\nNo feed URL specified in automatic.conf!\n\n");
		shutdown_daemon();
	}

	/* clear logfile */
	if(!nofork) {
		fp = fopen(log_file, "w");
		if(fp == NULL) {
			fprintf(stderr, "[main] Failed to open logfile '%s': %s\n", log_file, strerror(errno));
			shutdown_daemon();
		} else {
			fclose(fp);
		}
	}
	dbg_printf(P_INFO, "verbose: %d\n", verbose);
	dbg_printf(P_INFO, "nofork: %d\n", nofork);
	dbg_printf(P_INFO, "config file: %s\n", configfile);
	dbg_printf(P_INFO, "Transmission home: %s\n", config_path);
	dbg_printf(P_INFO, "check interval: %d min\n", check_interval);
	dbg_printf(P_INFO, "log file: %s\n", nofork ? "stderr" : log_file);
	dbg_printf(P_INFO, "state file: %s\n", state_file);
	dbg_printf(P_INFO, "Feed URL: %s\n", feed_url);
	dbg_printf(P_MSG, "Read %d patterns from config file\n", listCount(regex_patterns));
	print_list(regex_patterns);
	load_state(&bucket);
	setup_signals();

 	if(!nofork) {
		if(daemonize() != 0) {
			fprintf(stderr, "Error: Daemonize failed. Aborting...\n");
			shutdown_daemon();
			exit(1);
		}
		getlogtime_str(time_str);
		dbg_printf( P_MSG, "%s: Daemon started\n", time_str);
	}
	while(1) {
		getlogtime_str(time_str);
		dbg_printf( P_MSG, "------ %s: Checking for new episodes ------\n", time_str);
		wdata = getHTTPData(feed_url);
		if(wdata && wdata->response) {
			if(wdata->response->size > 0) {
				parse_xmldata(wdata->response->data, wdata->response->size);
			}
			WebData_free(wdata);
			dbg_printf(P_INFO2, "[main] freed 'data'\n");
			check_for_downloads();
		}
		cleanup_list(&rss_items);
 		sleep(check_interval * 60);
	}
	return 0;
}
