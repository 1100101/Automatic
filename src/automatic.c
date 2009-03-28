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

#define AM_DEFAULT_CONFIGFILE      "/etc/automatic.conf"
#define AM_DEFAULT_STATEFILE       ".automatic.state"
#define AM_DEFAULT_VERBOSE         P_MSG
#define AM_DEFAULT_NOFORK          0
#define AM_DEFAULT_MAXBUCKET       30
#define AM_DEFAULT_USETRANSMISSION 1
#define AM_DEFAULT_STARTTORRENTS   1

#define FROM_XML_FILE 0
#define TESTING 0

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <getopt.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <fcntl.h>     /* open */

#include "version.h"
#include "web.h"
#include "output.h"
#include "config_parser.h"
#include "xml_parser.h"
#include "state.h"
#include "utils.h"
#include "feed_item.h"
#include "file.h"
#include "downloads.h"
#include "transmission.h"
#include "torrent.h"

static char AutoConfigFile[MAXPATHLEN + 1];
static void session_free(auto_handle *as);

uint8_t closing = 0;
uint8_t verbose = AM_DEFAULT_VERBOSE;
uint8_t nofork  = AM_DEFAULT_NOFORK;

static void usage(void) {
  printf("usage: automatic [-fh] [-v level] [-c file]\n"
    "\n"
    "Automatic %s\n"
    "\n"
    "  -f --nodeamon             Run in the foreground and log to stderr\n"
    "  -h --help                 Display this message\n"
    "  -v --verbose <level>      Set output level to <level> (default=1)\n"
    "  -c --configfile <path>    Path to configuration file\n"
    "\n", LONG_VERSION_STRING );
  exit(0);
}

static void readargs(int argc, char ** argv, char **c_file, uint8_t * nofork,
    uint8_t * verbose) {
  char optstr[] = "fhv:c:";
  struct option longopts[] = { { "verbose", required_argument, NULL, 'v' }, {
      "nodaemon", no_argument, NULL, 'f' }, { "help", no_argument, NULL, 'h' },
      { "configfile", required_argument, NULL, 'c' }, { NULL, 0, NULL, 0 } };
  int opt;

  while (0 <= (opt = getopt_long(argc, argv, optstr, longopts, NULL ))) {
    switch (opt) {
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

static void shutdown_daemon(auto_handle *as) {
  char time_str[TIME_STR_SIZE];
  dbg_printf(P_MSG, "%s: Shutting down daemon", getlogtime_str(time_str));
  if (as && as->bucket_changed) {
    save_state(as->statefile, as->downloads);
  }
  session_free(as);
  exit(EXIT_SUCCESS);
}

static int daemonize(void) {
  int fd;

  if (getppid() == 1) {
    return -1;
  }

  switch (fork()) {
    case 0:
      break;
    case -1:
      fprintf(stderr, "Error daemonizing (fork)! %d - %s", errno, strerror(
          errno));
      return -1;
    default:
      _exit(0);
  }

  if (setsid() < 0) {
    fprintf(stderr, "Error daemonizing (setsid)! %d - %s", errno, strerror(
        errno));
    return -1;
  }

  switch (fork()) {
    case 0:
      break;
    case -1:
      fprintf(stderr, "Error daemonizing (fork2)! %d - %s\n", errno, strerror(
          errno));
      return -1;
    default:
      _exit(0);
  }

  fd = open("/dev/null", O_RDONLY);
  if (fd != 0) {
    dup2(fd, 0);
    close(fd);
  }

  fd = open("/dev/null", O_WRONLY);
  if (fd != 1) {
    dup2(fd, 1);
    close(fd);
  }

  fd = open("/dev/null", O_WRONLY);
  if (fd != 2) {
    dup2(fd, 2);
    close(fd);
  }
  return 0;
}

static void signal_handler(int sig) {
  switch (sig) {
    case SIGINT:
    case SIGTERM: {
      dbg_printf(P_INFO2, "SIGTERM/SIGINT caught");
      closing = 1;
      break;
    }
  }
}

static void setup_signals(void) {
  signal(SIGCHLD, SIG_IGN);       /* ignore child       */
  signal(SIGTSTP, SIG_IGN);       /* ignore tty signals */
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTERM, signal_handler); /* catch kill signal */
  signal(SIGINT , signal_handler); /* catch kill signal */
}

uint8_t am_get_verbose(void) {
  return verbose;
}

auto_handle* session_init(void) {
  char path[MAXPATHLEN];
  char *home;

  auto_handle *ses = am_malloc(sizeof(auto_handle));

  /* numbers */
  ses->max_bucket_items     = AM_DEFAULT_MAXBUCKET;
  ses->bucket_changed       = 0;
  ses->check_interval       = AM_DEFAULT_INTERVAL;
  ses->use_transmission     = AM_DEFAULT_USETRANSMISSION;
  ses->start_torrent        = AM_DEFAULT_STARTTORRENTS;
  ses->transmission_version = AM_TRANSMISSION_1_3;
  ses->rpc_port             = AM_DEFAULT_RPCPORT;

  /* strings */
  ses->transmission_path     = get_tr_folder();
  ses->torrent_folder        = get_temp_folder();
  ses->host                  = NULL;
  ses->auth                  = NULL;
  home = get_home_folder();
  sprintf(path, "%s/%s", home, AM_DEFAULT_STATEFILE);
  am_free(home);
  ses->statefile             = am_strdup(path);

  /* lists */
  ses->filters               = NULL;
  ses->feeds                 = NULL;
  ses->downloads             = NULL;

  return ses;
}

static void session_free(auto_handle *as) {
  if (as) {
    am_free(as->transmission_path);
    am_free(as->statefile);
    am_free(as->torrent_folder);
    am_free(as->host);
    am_free(as->auth);
    freeList(&as->feeds, feed_free);
    freeList(&as->downloads, NULL);
    freeList(&as->filters, NULL);
    am_free(as);
  }
}

static WebData* downloadTorrent(const char* url) {
  return getHTTPData(url);
}

static uint8_t addTorrentToTM(const auto_handle *ah, const void* t_data,
                           uint32_t t_size, const char *fname) {
  uint8_t success = 0;
  if (!ah->use_transmission) {
    if(saveFile(fname, t_data, t_size) == 0) {
      success = 1;
    }
  } else if (ah->transmission_version == AM_TRANSMISSION_1_2) {
    if (saveFile(fname, t_data, t_size) == 0) {
      if (call_transmission(ah->transmission_path, fname) == -1) {
        dbg_printf(P_ERROR, "[addTorrentToTM] error adding torrent '%s' to Transmission");
      } else {
        success = 1;
      }
      unlink(fname);
    }
  } else if (ah->transmission_version == AM_TRANSMISSION_1_3) {
    if (uploadTorrent(t_data, t_size, (ah->host != NULL) ? ah->host : AM_DEFAULT_HOST,
                      ah->auth, ah->rpc_port, ah->start_torrent) == 0) {
      success = 1;
    }
  }
  return success;
}

static void processRSSList(auto_handle *session, const simple_list items) {

  simple_list current_item = items;
  WebData *torrent = NULL;
  char fname[MAXPATHLEN];

  while(current_item && current_item->data) {
    feed_item item = (feed_item)current_item->data;
    if (isMatch(session->filters, item->name) && !has_been_downloaded(session->downloads, item->url)) {
      dbg_printf(P_MSG, "Found new download: %s (%s)", item->name, item->url);
      torrent = downloadTorrent(item->url);
      if(torrent) {
        get_filename(fname, torrent->content_filename, torrent->url, session->torrent_folder);
        /* add torrent to Transmission */
        if (addTorrentToTM(session, torrent->response->data, torrent->response->size, fname) == 1) {
          /* add torrent url to bucket list */
          if (addToBucket(torrent->url, &session->downloads, session->max_bucket_items) == 0) {
            session->bucket_changed = 1;
          } else {
            dbg_printf(P_ERROR, "Error: Unable to add matched download to bucket list: %s", torrent->url);
          }
        }
        WebData_free(torrent);
      }
    }
    current_item = current_item->next;
  }
}

static uint16_t processFeed(auto_handle *session, const rss_feed feed, uint8_t firstrun) {
  WebData *wdata = NULL;
  uint32_t item_count = 0;
  wdata = getHTTPData(feed->url);
  if (wdata && wdata->response) {
    simple_list items = parse_xmldata(wdata->response->data, wdata->response->size, &item_count, &feed->ttl);
    if(firstrun) {
      session->max_bucket_items += item_count;
      dbg_printf(P_INFO2, "History bucket size changed: %d", session->max_bucket_items);
    }
    processRSSList(session, items);
    freeList(&items, freeFeedItem);
  }
  WebData_free(wdata);
  return item_count;
}

/* main function */
int main(int argc, char **argv) {
  auto_handle *session = NULL;
  char *config_file = NULL;
#if FROM_XML_FILE
  char *xmldata = NULL;
  int fileLen = 0;
#endif
  int daemonized = 0;
  char erbuf[100];
  char time_str[TIME_STR_SIZE];
  NODE *current = NULL;
  uint32_t count = 0;
  uint8_t first_run = 1;

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

  if(listCount(session->feeds) == 0) {
    fprintf(stderr, "No feed URL specified in automatic.conf!\n");
    shutdown_daemon(session);
  }

  if(listCount(session->filters) == 0) {
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
  dbg_printf(P_INFO, "foreground mode: %s", nofork == 1 ? "yes" : "no");
  dbg_printf(P_INFO, "config file: %s", AutoConfigFile);
  dbg_printf(P_INFO, "Transmission home: %s", session->transmission_path);
  dbg_printf(P_INFO, "check interval: %d min", session->check_interval);
  dbg_printf(P_INFO, "torrent folder: %s", session->torrent_folder);
  dbg_printf(P_INFO, "start torrents: %s", session->start_torrent == 1 ? "yes" : "no");
  dbg_printf(P_INFO, "state file: %s", session->statefile);
  dbg_printf(P_MSG, "%d feed URL%s", listCount(session->feeds), listCount(session->feeds) > 
  1 ?  "s" : "");
  dbg_printf(P_MSG, "Read %d patterns from config file", listCount(session->filters));

  load_state(session->statefile, &session->downloads);
  while(!closing) {
    getlogtime_str(time_str);
    dbg_printf( P_INFO2, "------ %s: Checking for new episodes ------", time_str);
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
      ++count;
      dbg_printf(P_INFO2, "Checking feed %d ...", count);
      processFeed(session, current->data, first_run);
      current = current->next;
    }
    if(first_run) {
      dbg_printf(P_INFO2, "New bucket size: %d", session->max_bucket_items);
    }
    first_run = 0;
#endif
    sleep(session->check_interval * 60);
  }
  shutdown_daemon(session);
  return 0;
}
