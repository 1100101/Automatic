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


#define AM_LOCKFILE  					"/tmp/automatic.pid"
#define AM_DEFAULT_CONFIGFILE  		"/etc/automatic.conf"
#define AM_DEFAULT_LOGFILE  			"/var/log/automatic.log"
#define AM_DEFAULT_STATEFILE  		".automatic.state"
#define AM_DEFAULT_VERBOSE 			P_MSG
#define AM_DEFAULT_NOFORK 				0
#define AM_DEFAULT_MAXBUCKET 			10
#define AM_DEFAULT_USETRANSMISSION 	1
#define AM_DEFAULT_INTERVAL 			10

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <pwd.h>
#include <getopt.h>
#include <sys/types.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <sys/wait.h>

#include "version.h"
#include "web.h"
#include "output.h"
#include "config_parser.h"
#include "xml_parser.h"
#include "state.h"

static char AutoConfigFile[MAXPATHLEN + 1];
static void ah_free(auto_handle *as);

static linked_list rss_items = NULL;

static auto_handle *session;
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

char* get_home_folder() {
	char * dir = NULL;
	struct passwd * pw = NULL;

	if(!dir) {
		if(getenv("HOME")) {
			dir = strdup(getenv( "HOME" ));
		} else {
			if(pw = getpwuid(getuid())) {
				dir = strdup(pw->pw_dir);
				endpwent();
			} else {
				dir = strdup("");
			}
		}
	}
	return dir;
}

char* resolve_path(char *path) {
	char new_dir[MAXPATHLEN];
	char *homedir = NULL;

	if(path && strlen(path) > 2) {
		/* home dir */
		if(path[0] == '~' && path[1] == '/') {
			homedir = get_home_folder();
			if(homedir) {
				strcpy(new_dir, homedir);
				strcat(new_dir, ++path);
				am_free(homedir);
				return strdup(new_dir);
			}
		}
		return strdup(path);
	}
	return NULL;
}

char* get_tr_folder() {
	static char *path = NULL;
	char buf[MAXPATHLEN];

	if(!path) {
		strcpy(buf, get_home_folder());
		strcat(buf, "/.config/transmission");
		path = strdup(buf);
	}
	return path;
}

char* get_temp_folder() {
	static char *dir = NULL;

	if(!dir) {
		if(getenv("TEMPDIR")) {
			dir = strdup(getenv( "TEMPDIR" ));
		} else if(getenv("TMPDIR")) {
			dir = strdup(getenv( "TMPDIR" ));
		} else {
			dir = strdup("/tmp");
		}
	}
	fprintf(stderr, "dir8: %p\n", (void*)dir);
	return dir;
}

static uint8_t has_been_downloaded(linked_list bucket, char *url) {
	uint8_t res;

	res = hasURL(url, bucket);
	if(res)
		dbg_printf(P_INFO2, "Torrent has already been downloaded");
	return res;
}

static void check_for_downloads(void) {
	int err;
	regex_t preg;
	char erbuf[100];
	linked_list current_rss_item, current_regex;

	current_regex = session->regex_patterns;
	while (current_regex != NULL && current_regex->item != NULL) {
		dbg_printf(P_INFO2, "Current regex: %s", current_regex->item->name);
		err = regcomp(&preg, current_regex->item->name, REG_EXTENDED|REG_ICASE);
		if(err) {
			regerror(err, &preg, erbuf, sizeof(erbuf));
			dbg_printf(P_INFO2, "regcomp: Error compiling regular expression: %s", erbuf);
			current_regex = current_regex->pNext;
			continue;
		}
		current_rss_item = rss_items;
		while (current_rss_item != NULL && current_rss_item->item != NULL) {
			dbg_printf(P_INFO2, "Current rss_item: %s", current_rss_item->item->name);
			err = regexec(&preg, current_rss_item->item->name, 0, NULL, 0);
			if(!err && !has_been_downloaded(session->bucket, current_rss_item->item->url)) {			/* regex matches and it hasn't been downloaded before */
				download_torrent(session, current_rss_item->item);
			} else if(err != REG_NOMATCH && err != 0){
				regerror(err, &preg, erbuf, sizeof(erbuf));
				dbg_printf(P_ERROR, "[check_for_downloads] regexec error: %s", erbuf);
			}
			current_rss_item = current_rss_item->pNext;
		}
		regfree(&preg);
		current_regex = current_regex->pNext;
	}
}

static void do_cleanup(auto_handle *as) {
	cleanupList(&rss_items);
	ah_free(as);
	cd_preg_free();
}

void shutdown_daemon(auto_handle *as) {
	char time_str[TIME_STR_SIZE];
	getlogtime_str(time_str);
	dbg_printf(P_MSG,"%s: Shutting down daemon", time_str);
	if(as && as->bucket_changed)
		save_state(&as->bucket);
	do_cleanup(as);
	exit(0);
}

int daemonize() {
	int i, lfp;
	int fd;
	char str[10];

	if(getppid() == 1) {
		return -1;
	}

	switch(fork()) {
		case 0:
         break;
      case -1:
         dbg_printf(P_ERROR, "Error daemonizing (fork)! %d - %s", errno, strerror(errno));
         return -1;
      default:
         _exit(0);
	}

	if( setsid() < 0 ) {
      dbg_printf(P_ERROR, "Error daemonizing (setsid)! %d - %s", errno, strerror(errno) );
      return -1;
	}

	/* first instance continues */
	lfp = open(AM_LOCKFILE, O_RDWR|O_CREAT, 0640);

	if (lfp < 0) {
		dbg_printf(P_ERROR, "Failed to open lockfile (%s). Aborting...\n", strerror(errno));
		exit(1); /* can not open */
	}

	if (lockf(lfp,F_TLOCK,0) < 0) {
		dbg_printf(P_ERROR, "Cannot lock pid file. Another instance running? Aborting...\n");
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

	auto_handle *ses = am_malloc(sizeof(auto_handle));
	ses->max_bucket_items = AM_DEFAULT_MAXBUCKET;
	ses->bucket_changed = 0;
	ses->check_interval = AM_DEFAULT_INTERVAL;
	ses->use_transmission = AM_DEFAULT_USETRANSMISSION;
	ses->log_file = strdup(AM_DEFAULT_LOGFILE);
	ses->transmission_path = get_tr_folder();
	sprintf(path, "%s/%s", get_home_folder(), AM_DEFAULT_STATEFILE);
	ses->statefile = am_malloc(strlen(path) + 1);
	strncpy(ses->statefile, path, strlen(path) + 1);
	ses->torrent_folder = get_temp_folder();
	ses->bucket = NULL;
	ses->regex_patterns = NULL;
	ses->feed_url = NULL;
	return ses;
}

static void ah_free(auto_handle *as) {
	if(as) {
		am_free(as->feed_url);
		am_free(as->log_file);
		am_free(as->transmission_path);
		am_free(as->statefile);
		am_free(as->torrent_folder);
		cleanupList(&as->bucket);
		cleanupList(&as->regex_patterns);
		am_free(as);
	}
}

uint8_t am_get_verbose() {
	return verbose;
}

const char *am_get_statefile() {
	if(session && session->statefile)
/* 		return strdup(session->statefile); */
		return session->statefile;
	else
		return NULL;
}

const char* am_getlogfile() {
	if(session && session->log_file) {
		return session->log_file;
	} else
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

void am_set_bucket_size(uint8_t size) {
	if(session) {
		dbg_printf(P_INFO, "New bucket list size: %d", size);
 		session->max_bucket_items = size;
	}
}

void* am_malloc( size_t size ) {
	void *tmp;
	if(size > 0) {
		tmp = malloc(size);
		dbg_printf(P_DBG, "Allocated %d bytes (%p)", size, tmp);
		return tmp;
	}
	return NULL;
}

void* am_realloc(void *p, size_t size) {
	if(!p)
		return am_malloc(size);
	else
		return realloc(p, size);
}

void am_free(void * p) {
	if(p) {
		/*fprintf(stderr, "Freeing %p\n", p);*/
		free(p);
		p = NULL;
	}
}

int main(int argc, char **argv) {
	FILE *fp;

   char* config_file = NULL;
	int daemonized = 0;
	char erbuf[100];
	char time_str[TIME_STR_SIZE];
	WebData *wdata;

	readargs(argc, argv, &config_file, &nofork, &verbose);

	if(!config_file) {
		config_file = strdup(AM_DEFAULT_CONFIGFILE);
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

	if(!session->feed_url || strlen(session->feed_url) < 1)  {
		fprintf(stderr, "No feed URL specified in automatic.conf!\n");
		shutdown_daemon(session);
	}

	if(!session->regex_patterns)  {
		fprintf(stderr, "No patterns specified in automatic.conf!\n");
		shutdown_daemon(session);
	}

	setup_signals();

 	if(!nofork) {
		/* clear logfile */
		fp = fopen(session->log_file, "w");
		if(fp == NULL) {
			fprintf(stderr, "FATAL: Could not open logfile '%s': %s\n", session->log_file, strerror(errno));
			shutdown_daemon(session);
		} else {
			fclose(fp);
		}

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
	dbg_printf(P_INFO, "nofork: %s", nofork == 1 ? "true" : "false");
	dbg_printf(P_INFO, "config file: %s", AutoConfigFile);
	dbg_printf(P_INFO, "Transmission home: %s", session->transmission_path);
	dbg_printf(P_INFO, "check interval: %d min", session->check_interval);
	dbg_printf(P_INFO, "torrent folder: %s", session->torrent_folder);
	dbg_printf(P_INFO, "log file: %s", nofork ? "stderr" : session->log_file);
	dbg_printf(P_INFO, "state file: %s", session->statefile);
	dbg_printf(P_INFO, "feed URL: %s", session->feed_url);
	dbg_printf(P_MSG, "Read %d patterns from config file", listCount(session->regex_patterns));

	load_state(&session->bucket);

	while(1) {
		getlogtime_str(time_str);
		dbg_printf( P_MSG, "------ %s: Checking for new episodes ------", time_str);
		wdata = getHTTPData(session->feed_url);
		if(wdata && wdata->response && wdata->response->size > 0) {
			parse_xmldata(wdata->response->data, wdata->response->size, &rss_items);
		}

		WebData_free(wdata);
		check_for_downloads();
		cleanupList(&rss_items);
 		sleep(session->check_interval * 60);
	}
	return 0;
}
