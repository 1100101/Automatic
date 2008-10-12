/*
 * Copyright (C) 2008 Frank Aurich (1100101+automatic@gmail.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */


#define AM_DEFAULT_CONFIGFILE 			"/etc/automatic.conf"
#define AM_DEFAULT_STATEFILE  			".automatic.state"
#define AM_DEFAULT_VERBOSE 				P_MSG
#define AM_DEFAULT_NOFORK 				0
#define AM_DEFAULT_MAXBUCKET 			10
#define AM_DEFAULT_USETRANSMISSION 	    1

#define MAX_URL_LEN 256

#define FROM_XML_FILE 0
#define TESTING 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <getopt.h>
#include <regex.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <fcntl.h> /* open */

#include "version.h"
#include "web.h"
#include "output.h"
#include "config_parser.h"
#include "xml_parser.h"
#include "state.h"
#include "utils.h"
#include "feed_item.h"
#include "base64.h"
#include "file.h"

static char AutoConfigFile[MAXPATHLEN + 1];
static void ah_free(auto_handle *as);

static void addTorrentToTM(auto_handle *ah, const char *url);
static int call_transmission(auto_handle *ah, const char *filename);
static void get_filename(WebData *data, char *file_name, const char *path);
static int is_torrent(const char *str);
static uint8_t uploadTorrent(auto_handle *ses, void* t_data, int t_size);

static auto_handle *session = NULL;
uint8_t verbose = AM_DEFAULT_VERBOSE;
uint8_t nofork = AM_DEFAULT_NOFORK;

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

void readargs( int argc, char ** argv, char **c_file, uint8_t * nofork, uint8_t * verbose) {
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
				*verbose = atoi(optarg);
				break;
			case 'f':
				*nofork = 1;
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

static uint8_t has_been_downloaded(simple_list bucket, char *url) {
	uint8_t res;

	res = bucket_hasURL(url, bucket);
	if(res)
		dbg_printf(P_INFO2, "Torrent has already been downloaded");
	return res;
}

static void do_cleanup(auto_handle *as) {
	ah_free(as);
	cd_preg_free();
}

void shutdown_daemon(auto_handle *as) {
	char time_str[TIME_STR_SIZE];
	dbg_printf(P_MSG,"%s: Shutting down daemon", getlogtime_str(time_str));
	if(as && as->bucket_changed)
		save_state(&as->downloads);
	do_cleanup(as);
	exit(0);
}


int daemonize() {
	int fd;

 	if(getppid() == 1) {
		return -1;
	}

	switch(fork()) {
		case 0:
         break;
      case -1:
         fprintf( stderr, "Error daemonizing (fork)! %d - %s", errno, strerror(errno));
         return -1;
      default:
         _exit(0);
	}

	if( setsid() < 0 ) {
      fprintf( stderr, "Error daemonizing (setsid)! %d - %s", errno, strerror(errno) );
      return -1;
	}

	switch( fork( ) ) {
			case 0:
					break;
			case -1:
					fprintf( stderr, "Error daemonizing (fork2)! %d - %s\n", errno, strerror(errno) );
					return -1;
			default:
					_exit(0);
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
	return 0;
}


void signal_handler(int sig) {
	switch(sig) {
		case SIGINT:
		case SIGTERM: {
			dbg_printf(P_INFO2,"SIGTERM/SIGINT caught");
			shutdown_daemon(session);
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

auto_handle* session_init(void) {
	char path[MAXPATHLEN];
	char *home;

	auto_handle *ses = am_malloc(sizeof(auto_handle));
	ses->max_bucket_items = AM_DEFAULT_MAXBUCKET;
	ses->bucket_changed = 0;
	ses->check_interval = AM_DEFAULT_INTERVAL;
	ses->use_transmission = AM_DEFAULT_USETRANSMISSION;
	ses->transmission_version = AM_TRANSMISSION_1_3;
	ses->transmission_path = get_tr_folder();
	ses->rpc_port = AM_DEFAULT_RPCPORT;
	home = get_home_folder();
	sprintf(path, "%s/%s", home, AM_DEFAULT_STATEFILE);
	ses->statefile = am_malloc(strlen(path) + 1);
	strncpy(ses->statefile, path, strlen(path) + 1);
	ses->torrent_folder = get_temp_folder();
	ses->downloads = NULL;
	ses->host = NULL;
	ses->auth = NULL;
	ses->filters = NULL;
	ses->feeds = NULL;
	am_free(home);
	return ses;
}

static void ah_free(auto_handle *as) {
	if(as) {
		am_free(as->transmission_path);
		am_free(as->statefile);
		am_free(as->torrent_folder);
		am_free(as->host);
		am_free(as->auth);
		freeList(&as->feeds, feed_free);
		freeList( &as->downloads, NULL);
		freeList(&as->filters, NULL);
		am_free(as);
	}
}

uint8_t am_get_verbose() {
	return verbose;
}

const char *am_get_statefile() {
	if(session && session->statefile)
		return session->statefile;
	else
		return NULL;
}

uint8_t am_get_nofork() {
	return nofork;
}

uint8_t am_get_bucket_size() {
	if(session)
		return session->max_bucket_items;
	else
		return AM_DEFAULT_MAXBUCKET;
}

int am_get_interval() {
	if(session)
		return session->check_interval;
	else
		return AM_DEFAULT_INTERVAL;
}

void am_set_interval(int interval) {
	if(session) {
		dbg_printf(P_INFO, "New interval: %d", interval);
		session->check_interval = interval;
	}
}

void am_set_bucket_size(uint8_t size) {
	if(session) {
		dbg_printf(P_INFO, "New bucket list size: %d", size);
 		session->max_bucket_items = size;
	}
}

void findMatch(feed_item item) {
	int err;
	regex_t preg;
	char erbuf[100];
	char *regex_str;
	simple_list current_regex;

	assert(session != NULL);

	current_regex = session->filters;
	while (current_regex != NULL && current_regex->data != NULL) {
		regex_str = (char*)current_regex->data;
		dbg_printf(P_INFO2, "Current regex: %s", regex_str);
		err = regcomp(&preg, regex_str, REG_EXTENDED|REG_ICASE);
		if(err) {
			regerror(err, &preg, erbuf, sizeof(erbuf));
			dbg_printf(P_INFO2, "regcomp: Error compiling regular expression: %s", erbuf);
			current_regex = current_regex->next;
			continue;
		}
		dbg_printf(P_INFO2, "Current feed_item: %s", item->name);
		err = regexec(&preg, item->name, 0, NULL, 0);
		if(!err && !has_been_downloaded(session->downloads, item->url)) {			/* regex matches and it hasn't been downloaded before */
			dbg_printf(P_MSG, "Found new download: %s (%s)", item->name, item->url);
			addTorrentToTM(session, item->url);
		} else if(err != REG_NOMATCH && err != 0){
			regerror(err, &preg, erbuf, sizeof(erbuf));
			dbg_printf(P_ERROR, "[check_for_downloads] regexec error: %s", erbuf);
		}
		regfree(&preg);
		current_regex = current_regex->next;
	}
}


/*
int main(int argc, char **argv) {
	char *data = NULL;
	uint32_t fileLen = 0;
	uint32_t ret;

	static auto_handle *session;

	session = session_init();
	session->rpc_port = 31339;
	session->auth = am_strdup("KyleK:testpw");

	dbg_printf(P_INFO, "Transmission version: 1.%d", session->transmission_version);
	dbg_printf(P_INFO, "RPC port: %d", session->rpc_port);
	dbg_printf(P_INFO, "RPC auth: %s", session->auth != NULL ? session->auth : "none");


	// Loading torrent from file
	data = readFile("test.torrent", &fileLen);
	if(data != NULL && fileLen > 0) {
		ret = uploadTorrent(session, data, fileLen);
		am_free(data);
	}
	do_cleanup(session);
	return 0;
}
*/

int main(int argc, char **argv) {
  char* config_file = NULL;
#if FROM_XML_FILE
	char *xmldata = NULL;
	int fileLen = 0;
#endif
	int daemonized = 0;
	int first_run = 1;
	uint32_t ret;
	char erbuf[100];
	char time_str[TIME_STR_SIZE];
	WebData *wdata = NULL;
	NODE *current = NULL;
	int count;
	int status;
	rss_feed feed;

	readargs(argc, argv, &config_file, &nofork, &verbose);

	if(!config_file) {
		config_file = am_strdup(AM_DEFAULT_CONFIGFILE);
	}
	strncpy(AutoConfigFile, config_file, strlen(config_file));

	session = session_init();

	if(parse_config_file(session, AutoConfigFile) != 0) {
		if(errno == ENOENT) {
			snprintf(erbuf, sizeof(erbuf), "Cannot find file '%s'", config_file);
		} else {
			snprintf(erbuf, sizeof(erbuf), "Unknown error");
		}
		fprintf(stderr, "Error parsing config file: %s\n", erbuf);
		exit(1);
	}

	if(listCount(session->feeds) == 0)  {
		fprintf(stderr, "No feed URL specified in automatic.conf!\n");
		shutdown_daemon(session);
	}

	if(listCount(session->filters) == 0)  {
		fprintf(stderr, "No patterns specified in automatic.conf!\n");
		shutdown_daemon(session);
	}

	setup_signals();

 	if(!nofork) {

		/* start daemon */
		if(daemonize() != 0) {
			dbg_printf(P_ERROR, "Error: Daemonize failed. Aborting...");
			shutdown_daemon(session);
		}
		daemonized = 1;
		getlogtime_str(time_str);
		dbg_printf( P_MSG, "%s: Daemon started", time_str);
	}

	dbg_printf(P_INFO, "verbose level: %d", verbose);
	dbg_printf(P_INFO, "Transmission version: 1.%d", session->transmission_version);
	dbg_printf(P_INFO, "RPC host: %s", session->host != NULL ? session->host : AM_DEFAULT_HOST);
	dbg_printf(P_INFO, "RPC port: %d", session->rpc_port);
	dbg_printf(P_INFO, "RPC auth: %s", session->auth != NULL ? session->auth : "none");
	dbg_printf(P_INFO, "foreground mode: %s", nofork == 1 ? "true" : "false");
	dbg_printf(P_INFO, "config file: %s", AutoConfigFile);
	dbg_printf(P_INFO, "Transmission home: %s", session->transmission_path);
	dbg_printf(P_INFO, "check interval: %d min", session->check_interval);
	dbg_printf(P_INFO, "torrent folder: %s", session->torrent_folder);
	dbg_printf(P_INFO, "state file: %s", session->statefile);
	dbg_printf(P_MSG,  "%d feed URLs", listCount(session->feeds));
	dbg_printf(P_MSG,  "Read %d patterns from config file", listCount(session->filters));


	status = fork();

	if(status == -1) {
		fprintf( stderr, "Error forking for watchdog! %d - %s", errno, strerror(errno));
	} else if(status == 0) {		/* folder watch process */
		while(1) {
			printf("folder watch\n");
			sleep(5);
		}
	} else {		/* RSS feed watching process */
		load_state(&session->downloads);
		while(1) {
			getlogtime_str(time_str);
			dbg_printf( P_MSG, "------ %s: Checking for new episodes ------", time_str);
	#if FROM_XML_FILE
			xmldata = readFile("feed.xml", &fileLen);
			if(xmldata != NULL) {
				fileLen = strlen(xmldata);
				parse_xmldata(xmldata, fileLen);
				am_free(xmldata);
			}
	#else
			current = session->feeds;
			count = 0;
			while(current && current->data) {
				feed = (rss_feed)current->data;
				++count;
				dbg_printf(P_MSG, "Checking feed %d ...", count);
				wdata = getHTTPData(feed->url);
				if(wdata && wdata->response && wdata->response->size > 0) {
					ret = parse_xmldata(wdata->response->data, wdata->response->size);
					if(first_run == 1 && ret != -1) {
						if(count == 1) {            /* first feed count replaces default value */
							session->max_bucket_items = ret;
						} else {
							session->max_bucket_items += ret;
						}
						dbg_printf(P_INFO2, "New bucket size: %d", session->max_bucket_items);
					}
				}
				WebData_free(wdata);
				current = current->next;
			}
			first_run = 0;
	#endif
			sleep(session->check_interval * 60);
		}
	}
	return 0;
}

static void processTorrent(auto_handle *ah, WebData *wdata, const char *torrent_url) {
	char fname[MAXPATHLEN];
	int res, success = 0;

	if(!ah->use_transmission) {
		get_filename(wdata, fname, ah->torrent_folder);
		res = saveFile(fname, wdata->response->data, wdata->response->size);
		if(res == 0) {
			success = 1;
		}
	} else if(ah->transmission_version == AM_TRANSMISSION_1_2) {
		get_filename(wdata, fname, ah->torrent_folder);
		res = saveFile(fname, wdata->response->data, wdata->response->size);
		if(res == 0) {
			if(call_transmission(ah, fname) == -1) {
				dbg_printf(P_ERROR, "[processTorrent] error adding torrent '%s' to Transmission");
			} else {
				success = 1;
			}
			unlink(fname);
		}
	} else if(ah->transmission_version == AM_TRANSMISSION_1_3) {
		if(uploadTorrent(ah, wdata->response->data, wdata->response->size) == 0) {
			success = 1;
		}
	}

	if(success == 1) {
		if(addToBucket(torrent_url, &ah->downloads, ah->max_bucket_items) == 0) {
			ah->bucket_changed = 1;
		} else {
			dbg_printf(P_ERROR, "Error: Unable to add matched download to bucket list: %s", torrent_url);
		}
	}
}

static void addTorrentToTM(auto_handle *ah, const char *url) {
	WebData *wdata;

	wdata = getHTTPData(url);
	if(wdata != NULL && wdata->response != NULL) {
		processTorrent(ah, wdata, url);
	}
	WebData_free(wdata);
}


static int call_transmission(auto_handle *ah, const char *filename) {
	char buf[MAXPATHLEN];
	int8_t status;

	if(filename && strlen(filename) > 1) {
		snprintf(buf, MAXPATHLEN, "transmission-remote -g \"%s\" -a \"%s\"", ah->transmission_path, filename);
		dbg_printf(P_INFO, "[call_transmission] calling transmission-remote...");
		dbg_printf(P_INFO2, "[call_transmission] call: %s", buf);
		status = system(buf);
		dbg_printf(P_DBG, "[call_transmission] WEXITSTATUS(status): %d", WEXITSTATUS(status));
		if(status == -1) {
			dbg_printf(P_ERROR, "\n[call_transmission] Error calling sytem(): %s", strerror(errno));
			return -1;
		} else {
			dbg_printf(P_INFO, "[call_transmission] Success");
			return 0;
		}
	} else {
		dbg_printf(P_ERROR, "[call_transmission] Error: invalid filename (addr: %p)", (void*)filename);
		return -1;
	}
}


static void get_filename(WebData *data, char *file_name, const char *path) {
	char *p, tmp[MAXPATHLEN], fname[MAXPATHLEN], buf[MAXPATHLEN];
	int len;

#ifdef DEBUG
	assert(data);
	assert(path);
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
	strcpy(fname, path);
	strcat(fname, "/");
	strcat(fname, buf);
	if(!is_torrent(buf)) {
		strcat(fname, ".torrent");
	}
	strcpy(file_name, fname);
}

static int is_torrent(const char *str) {
	if(strstr(str, ".torrent"))
		return 1;
	else
		return 0;
}

static char* makeJSON(void *data, uint32_t tsize, uint32_t *setme_size) {

	char *encoded = NULL;
	uint32_t enc_size, json_size;
	char *buf = NULL;
	int buf_size;
	const char *JSONstr =
		"{\n"
		"\"method\": \"torrent-add\",\n"
		"\"arguments\": {\n"
		"\"metainfo\": \"%s\"\n"
		"}\n"
		"}";

	if(data != NULL) {
		encoded = base64_encode(data, tsize, &enc_size);
		if(encoded && enc_size > 0) {
			buf_size = enc_size + 70;
			buf = am_malloc(buf_size);
			json_size = snprintf(buf, buf_size, JSONstr, encoded);
			if(json_size < 0 || json_size >= buf_size) {
				dbg_printf(P_ERROR, "Error producing JSON string with Base64-encoded metadata: %s", strerror(errno));
				am_free(encoded);
				am_free(buf);
				return NULL;
			}
			buf[json_size] = '\0';
			if(setme_size) {
				*setme_size = json_size;
			}
			am_free(encoded);
			return buf;
		}
	}
	return NULL;
}


static char* parseResponse(const char* response) {
	int err, len;
	char erbuf[100];
	regmatch_t pmatch[2];
 	const char* result_regex = "\"result\": \"(.+)\"\n";
	regex_t *result_preg = am_malloc(sizeof(regex_t));
	char *result_str = NULL;

	err = regcomp(result_preg, result_regex, REG_EXTENDED|REG_ICASE);
	if(err) {
		regerror(err, result_preg, erbuf, sizeof(erbuf));
		dbg_printf(P_ERROR, "[parseResponse] regcomp: Error compiling regular expression: %s", erbuf);
	}
	err = regexec(result_preg, response, 2, pmatch, 0);
	if(!err) {			/* regex matches */
		len = pmatch[1].rm_eo - pmatch[1].rm_so;
		dbg_printf(P_DBG, "result len: %d (end: %d, start: %d)", len, pmatch[1].rm_eo, pmatch[1].rm_so);
		result_str = am_strndup(response + pmatch[1].rm_so, len);
	} else {
		len = regerror(err, result_preg, erbuf, sizeof(erbuf));
		dbg_printf(P_ERROR, "[parseResponse] regexec error: %s", erbuf);
	}
	regfree(result_preg);
	am_free(result_preg);
	return result_str;
}

static uint8_t uploadTorrent(auto_handle *ses, void* t_data, int t_size) {
	char *packet = NULL, *res = NULL, *response = NULL;
	uint32_t packet_size = 0, ret = -1;
	char url[MAX_URL_LEN];

	packet = makeJSON(t_data, t_size, &packet_size);
	if(packet != NULL) {
      snprintf( url, MAX_URL_LEN, "http://%s:%d/transmission/rpc", (ses->host != NULL) ? ses->host : AM_DEFAULT_HOST, ses->rpc_port);

      res = sendHTTPData(url, ses->auth, packet, packet_size);
      if(res != NULL) {
          response = parseResponse(res);
          if(!strncmp(response, "success", 7)) {
              dbg_printf(P_MSG, "Torrent upload successful!");
              ret = 0;
          }else if(!strncmp(response, "duplicate torrent", 17)) {
              dbg_printf(P_MSG, "Duplicate Torrent");
              ret = 0;
          } else {
              dbg_printf(P_ERROR, "Error uploading torrent: %s", response);
              ret = -1;
          }
          am_free(response);
          am_free(res);
      }
      am_free(packet);
	}
	return ret;
}
