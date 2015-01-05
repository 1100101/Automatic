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
#define AM_DEFAULT_NOFORK          false
#define AM_DEFAULT_MAXBUCKET       30
#define AM_DEFAULT_USETRANSMISSION 1
#define AM_DEFAULT_STARTTORRENTS   1

#define FROM_XML_FILE 0
#define TESTING 0

#include <assert.h>
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>     /* open */

#include "config_parser.h"
#include "downloads.h"
#include "feed_item.h"
#include "file.h"
#include "output.h"
#include "prowl.h"
#include "pushalot.h"
#include "toasty.h"
#include "regex.h"
#include "state.h"
#include "torrent.h"
#include "transmission.h"
#include "utils.h"
#include "version.h"
#include "web.h"
#include "xml_parser.h"

PRIVATE char AutoConfigFile[MAXPATHLEN + 1];
PRIVATE void session_free(auto_handle *as);
PRIVATE bool isMagnetURI(const char* uri);
PRIVATE auto_handle * mySession = NULL;
PRIVATE bool closing = false;
PRIVATE bool nofork  = AM_DEFAULT_NOFORK;
PRIVATE bool isRunning = false;
PRIVATE bool seenHUP = false;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE void usage(void) {
  printf("usage: automatic [-fh] [-v level] [-l logfile] [-c file] [-p pidfile]\n"
    "\n"
    "Automatic %s\n"
    "\n"
    "  -f --nodeamon             Run in the foreground and log to stderr\n"
    "  -h --help                 Display this message\n"
    "  -v --verbose <level>      Set output level to <level> (default=1)\n"
    "  -c --configfile <path>    Path to configuration file\n"
    "  -o --once                 Quit Automatic after first check of RSS feeds\n"
    "  -l --logfile <file>       Log messages to <file>\n"
    "  -a --append-log           Don't overwrite logfile from a previous session\n"
    "  -p --pidfile              create pidfile"
    "\n", LONG_VERSION_STRING );
  exit(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE void readargs(int argc, char ** argv, char **c_file, char** logfile, char** pidfile, char **xmlfile,
                      bool * nofork, uint8_t * verbose, uint8_t * once, uint8_t * append_log,
                      uint8_t * match_only) {
  char optstr[] = "afhv:c:l:p:ox:m";
  struct option longopts[] = {
    { "verbose",    required_argument, NULL, 'v' },
    { "nodaemon",   no_argument,       NULL, 'f' },
    { "help",       no_argument,       NULL, 'h' },
    { "configfile", required_argument, NULL, 'c' },
    { "once",       no_argument,       NULL, 'o' },
    { "logfile",    required_argument, NULL, 'l' },
    { "pidfile",    required_argument, NULL, 'p' },
    { "append-log", no_argument,       NULL, 'a' },
    { "xml",        required_argument, NULL, 'x' },
    { "match-only", no_argument,       NULL, 'm' },
    { NULL, 0, NULL, 0 } };
  int opt;

  while (0 <= (opt = getopt_long(argc, argv, optstr, longopts, NULL ))) {
    switch (opt) {
      case 'a':
        *append_log = 1;
        break;
      case 'v':
        *verbose = atoi(optarg);
        break;
      case 'f':
        *nofork = true;
        break;
      case 'c':
        *c_file = optarg;
        break;
      case 'l':
        *logfile = optarg;
        break;
      case 'p':
        *pidfile = optarg;
        break;
      case 'x':
        *xmlfile = optarg;
        *nofork = true;
        *once = 1;
        break;
      case 'o':
        *once = 1;
        break;
      case 'm':
        *match_only = 1;
        break;
      default:
        usage();
        break;
    }
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE void shutdown_daemon(auto_handle *as) {
  dbg_printft(P_MSG, "Shutting down daemon");
  if (as && as->bucket_changed) {
    save_state(as->statefile, as->downloads);
  }
  session_free(as);
  SessionID_free();
  pid_close();
  log_close();
  exit(EXIT_SUCCESS);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE int daemonize(const char* pidfile) {
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

  umask(0); /* change the file mode mask */

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

  if (pidfile && *pidfile && getpid()) {
    pid_create(pidfile, getpid());
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

auto_handle* session_init(void) {
  char path[MAXPATHLEN];
  char *home;

  am_session_t *ses = am_malloc(sizeof(am_session_t));

  /* numbers */
  ses->rpc_version          = AM_DEFAULT_RPC_VERSION;
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
  ses->prowl_key             = NULL;
  ses->toasty_key            = NULL;
  ses->pushalot_key          = NULL;
  ses->prowl_key_valid       = 0;
  ses->match_only            = 0;
  ses->transmission_external = NULL;

  /* lists */
  ses->filters              = NULL;
  ses->feeds                 = NULL;
  ses->downloads             = NULL;
  ses->upspeed               = -1;

  return ses;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE void printSessionSettings() {
  dbg_printf(P_INFO, "Transmission version: 1.%d", mySession->transmission_version);
  dbg_printf(P_INFO, "RPC host: %s", mySession->host != NULL ? mySession->host : AM_DEFAULT_HOST);
  dbg_printf(P_INFO, "RPC port: %d", mySession->rpc_port);
  dbg_printf(P_INFO, "RPC auth: %s", mySession->auth != NULL ? mySession->auth : "none");
  dbg_printf(P_INFO, "config file: %s", AutoConfigFile);
  dbg_printf(P_INFO, "Transmission home: %s", mySession->transmission_path);
  dbg_printf(P_INFO, "check interval: %d min", mySession->check_interval);
  dbg_printf(P_INFO, "Upload limit: %d KB/s", mySession->upspeed);
  dbg_printf(P_INFO, "torrent folder: %s", mySession->torrent_folder);
  dbg_printf(P_INFO, "start torrents: %s", mySession->start_torrent == 1 ? "yes" : "no");
  dbg_printf(P_INFO, "state file: %s", mySession->statefile);

  /* determine RPC version */
  if(mySession->use_transmission && mySession->transmission_version == AM_TRANSMISSION_1_3) {
      mySession->rpc_version = getRPCVersion((mySession->host != NULL) ? mySession->host : AM_DEFAULT_HOST,
                                            mySession->rpc_port, mySession->auth);
      if(mySession->rpc_version != 0) {
         dbg_printf(P_INFO, "Transmission RPC Version: %d", mySession->rpc_version);
      }
  }

  if(mySession->prowl_key) {
    dbg_printf(P_INFO, "Prowl API key: %s", mySession->prowl_key);
  }

  if(mySession->toasty_key) {
    dbg_printf(P_INFO, "Toasty DeviceID: %s", mySession->toasty_key);
  }

  if(mySession->pushalot_key) {
    dbg_printf(P_INFO, "Pushalot Token: %s", mySession->pushalot_key);
  }

  dbg_printf(P_MSG,  "%d feed URLs", listCount(mySession->feeds));
  dbg_printf(P_MSG,  "Read %d filters from config file", listCount(mySession->filters));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE bool setupSession(auto_handle * session) {
   bool sessionOk = true;

   if(session != NULL) {
      if(listCount(session->feeds) == 0) {
         dbg_printf(P_ERROR, "No feeds specified in automatic.conf!");
         sessionOk = false;
      }

      if(listCount(session->filters) == 0) {
         dbg_printf(P_ERROR, "No filters specified in automatic.conf!");
         sessionOk = false;
      }

      if(sessionOk) {
         // There's been a previous session.
         // Copy over some of its values, and properly free its memory.
         if(mySession != NULL) {
            session->match_only = mySession->match_only;
            if(mySession->bucket_changed) {
               save_state(mySession->statefile, mySession->downloads);
            }

            session_free(mySession);
         }

         mySession = session;

         /* check if Prowl API key is given, and if it is valid */
         if(mySession->prowl_key && verifyProwlAPIKey(mySession->prowl_key) == 1 ) {
            mySession->prowl_key_valid = 1;
         }

         load_state(mySession->statefile, &mySession->downloads);
         printSessionSettings();
      }
   } else {
      sessionOk = false;
   }

   return sessionOk;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE void signal_handler(int sig) {
  switch (sig) {
    case SIGINT:
    case SIGTERM: {
      dbg_printf(P_INFO2, "SIGTERM/SIGINT caught");
      closing = true;
      break;
    }
    case SIGHUP: {
      if(isRunning || !mySession) {
         seenHUP = true;
      } else {
         auto_handle * s = NULL;
         dbg_printf(P_MSG, "Caught SIGHUP. Reloading config file.");
         s = session_init();
         if((parse_config_file(s, AutoConfigFile) != 0) || !setupSession(s)) {
            dbg_printf(P_ERROR, "Error parsing config file. Keeping the old settings.");
            session_free(s);
         }

         seenHUP = false;
      }

      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE void setup_signals(void) {
  signal(SIGCHLD, SIG_IGN);       /* ignore child       */
  signal(SIGTSTP, SIG_IGN);       /* ignore tty signals */
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTERM, signal_handler); /* catch kill signal */
  signal(SIGINT , signal_handler); /* catch kill signal */
  signal(SIGHUP , signal_handler); /* catch hangup signal */
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE void session_free(auto_handle *as) {
  if (as) {
    am_free(as->transmission_path);
    as->transmission_path = NULL;
    am_free(as->torrent_folder);
    as->torrent_folder = NULL;
    am_free(as->statefile);
    as->statefile = NULL;
    am_free(as->host);
    as->host = NULL;
    am_free(as->auth);
    as->auth = NULL;
    am_free(as->prowl_key);
    as->prowl_key = NULL;
    am_free(as->toasty_key);
    as->toasty_key = NULL;
    am_free(as->pushalot_key);
    as->pushalot_key = NULL;
    am_free(as->transmission_external);
    as->transmission_external = NULL;
    freeList(&as->feeds, feed_free);
    freeList(&as->downloads, NULL);
    freeList(&as->filters, filter_free);
    am_free(as);
    as = NULL;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE HTTPResponse* downloadTorrent(CURL* curl_session, const char* url) {
   HTTPResponse * torrent = NULL;
   dbg_printf(P_INFO2, "[downloadTorrent] url=%s, curl_session=%p", url, (void*)curl_session);
   torrent = getHTTPData(url, NULL, &curl_session);
   if(torrent && torrent->responseCode != 200) {
      switch(torrent->responseCode) {
         case 401:
            dbg_printf(P_ERROR, "Error downloading torrent (HTTP error %d: Bad authentication)", torrent->responseCode);
            break;
         case 403:
           dbg_printf(P_ERROR, "Error downloading torrent (HTTP error %d: Forbidden)", torrent->responseCode);
           break;
         default:
           dbg_printf(P_ERROR, "Error downloading torrent (HTTP error %d)", torrent->responseCode);
      }

      HTTPResponse_free(torrent);
      torrent = NULL;
   }

   return torrent;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE int8_t addTorrentToTM(const auto_handle *ah, const void* t_data,
                           uint32_t t_size, const char *fname, const char *folder) {
  int8_t success = -1;
  torrent_id_t tid;
  char url[MAX_URL_LEN];

  if (!ah->use_transmission) {
    if(saveFile(fname, t_data, t_size) == 0) {
      success = 1;
    } else {
      success = -1;
    }
  } else if(ah->transmission_version == AM_TRANSMISSION_EXTERNAL) {
    /* transmssion version set to external */
    if (saveFile(fname, t_data, t_size) == 0) {
      if (call_external(ah->transmission_external, fname) != 0) {
        dbg_printf(P_ERROR, "[addTorrentToTM] %s: Error adding torrent to Transmission-external", fname);
        success = -1;
      } else {
        success = 1;
      }
      unlink(fname);
    } else {
      success = -1;
    }
  } else if (ah->transmission_version == AM_TRANSMISSION_1_2) {
    if (saveFile(fname, t_data, t_size) == 0) {
      if (call_transmission(ah->transmission_path, fname) == -1) {
        dbg_printf(P_ERROR, "[addTorrentToTM] %s: Error adding torrent to Transmission", fname);
      } else {
        success = 1;
      }
      unlink(fname);
    }
  } else if (ah->transmission_version == AM_TRANSMISSION_1_3) {
    snprintf( url, MAX_URL_LEN, "http://%s:%d/transmission/rpc",
            (ah->host != NULL) ? ah->host : AM_DEFAULT_HOST, ah->rpc_port);
    tid = uploadTorrent(t_data, t_size, url, ah->auth, ah->start_torrent, folder);
    if(tid > 0) {  /* tid > 0: torrent ID --> torrent was added to TM */
      success = 1;
      if(ah->upspeed > 0) {
        changeUploadSpeed(url, ah->auth, tid, ah->upspeed, ah->rpc_version);
      }
    } else if(tid == 0) {  /* duplicate torrent */
      success = 0;
    } else {      /* torrent was not added */
      success = -1;
    }
  }
  return success;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE int8_t addMagnetToTM(const auto_handle *ah, const char* magnet_uri, const char *folder) {
   int8_t success = -1;
   torrent_id_t tid;
   char url[MAX_URL_LEN];

   if (ah->use_transmission) {
     if (ah->transmission_version == AM_TRANSMISSION_1_3) {
        snprintf( url, MAX_URL_LEN, "http://%s:%d/transmission/rpc", (ah->host != NULL) ? ah->host : AM_DEFAULT_HOST, ah->rpc_port);
        tid = uploadMagnet(magnet_uri, url, ah->auth, ah->start_torrent, folder);
        if(tid > 0) {  /* tid > 0: torrent ID --> torrent was added to TM */
           success = 1;
           if(ah->upspeed > 0) {
             changeUploadSpeed(url, ah->auth, tid, ah->upspeed, ah->rpc_version);
           }
        } else if(tid == 0) {  /* duplicate torrent */
          success = 0;
        } else {      /* torrent was not added */
          success = -1;
        }
     } else {
       dbg_printf(P_ERROR, "[addMagnetToTM] Magnet Links only work with Transmission 1.3+");
     }
   } else {
     success = 1; // debug!
     dbg_printf(P_ERROR, "[addMagnetToTM] Magnet links only work with Transmission, but use of Transmission is disabled in configuration!");
   }
   return success;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE void processRSSList(auto_handle *session, CURL *curl_session, const simple_list items, const rss_feed * feed) {
  simple_list current_item = items;
  HTTPResponse *torrent = NULL;
  char fname[MAXPATHLEN];
  char *download_folder = NULL;
  char *feedID = NULL;
  char *download_url = NULL;

  if(!curl_session && !session) {
    printf("curl_session == NULL && session == NULL\n");
    abort();
  }

  if(feed != NULL) {
    feedID = feed->id;
  }

  while(current_item && current_item->data) {
    feed_item item = (feed_item)current_item->data;

    if(isMatch(session->filters, item->name, feedID, &download_folder)) {
      if(!session->match_only) {
         if(has_been_downloaded(session->downloads, item)) {
            dbg_printf(P_INFO, "Duplicate torrent: %s", item->name);
         } else {
            int8_t result = -1;
            dbg_printft(P_MSG, "[%s] Found new download: %s (%s)", feedID, item->name, item->url);
            if(isMagnetURI(item->url)) {
               result = addMagnetToTM(session, item->url, download_folder);
            } else {
               // It's a torrent file
               // Rewrite torrent URL, if necessary
               if((feed != NULL) && (feed->url_pattern != NULL) && (feed->url_replace != NULL)) {
                  download_url = rewriteURL(item->url, feed->url_pattern, feed->url_replace);
               }

               torrent = downloadTorrent(curl_session, download_url != NULL ? download_url : item->url);
               if(torrent) {
                  get_filename(fname, torrent->content_filename, item->url, session->torrent_folder);

                  /* add torrent to Transmission */
                  result = addTorrentToTM(session, torrent->data, torrent->size, fname, download_folder);
                  HTTPResponse_free(torrent);
               }

               am_free(download_url);
            }

            // process result
            if( result >= 0) {  //result == 0 -> duplicate torrent
               if(result > 0) { //torrent was added
                  if(session->prowl_key_valid) {
                     prowl_sendNotification(PROWL_NEW_DOWNLOAD, session->prowl_key, item->name);
                  }

                  if(session->toasty_key) {
                     toasty_sendNotification(PROWL_NEW_DOWNLOAD, session->toasty_key, item->name);
                  }

                  if(session->pushalot_key) {
                     pushalot_sendNotification(PUSHALOT_NEW_DOWNLOAD, session->pushalot_key, item->name);
                  }
               }

               /* add url to bucket list */
               result = addToBucket(item->guid != NULL ? item->guid : item->url, &session->downloads, session->max_bucket_items);
               if (result == 0) {
                  session->bucket_changed = 1;
                  save_state(session->statefile, session->downloads);
               }
            } else {  //an error occurred
               if(session->prowl_key_valid) {
                  prowl_sendNotification(PROWL_DOWNLOAD_FAILED, session->prowl_key, item->name);
               }

               if(session->toasty_key) {
                  toasty_sendNotification(PROWL_DOWNLOAD_FAILED, session->toasty_key, item->name);
               }

               if(session->pushalot_key) {
                  pushalot_sendNotification(PUSHALOT_DOWNLOAD_FAILED, session->pushalot_key, item->name);
               }
            }
         }
      } else {
         dbg_printft(P_MSG, "[%s] Match: %s (%s)", feedID, item->name, item->url);
      }
    }

    current_item = current_item->next;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE HTTPResponse* getRSSFeed(const rss_feed* feed, CURL **session) {
  return getHTTPData(feed->url, feed->cookies, session);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE uint16_t processFeed(auto_handle *session, rss_feed* feed, uint8_t firstrun) {
  HTTPResponse *response = NULL;
  CURL         *curl_session = NULL;
  uint32_t      item_count = 0;

  response = getRSSFeed(feed, &curl_session);
  dbg_printf(P_INFO2, "[processFeed] curl_session=%p", (void*)curl_session);

  if(!curl_session && response != NULL) {
    dbg_printf(P_ERROR, "curl_session == NULL but response != NULL");
    abort();
  }

  if (response) {
    if(response->responseCode == 200 && response->data) {
      simple_list items = parse_xmldata(response->data, response->size, &item_count, &feed->ttl);

      dbg_printf(P_INFO, "Checking feed '%s' (%d items)", feed->url, item_count);

      if(firstrun) {
        session->max_bucket_items += item_count;
        dbg_printf(P_INFO2, "History bucket size changed: %d", session->max_bucket_items);
      }

      processRSSList(session, curl_session, items, feed);
      freeList(&items, freeFeedItem);
    }

    HTTPResponse_free(response);
    closeCURLSession(curl_session);
  }

  return item_count;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE uint16_t processFile(auto_handle *session, const char* xmlfile) {
  uint32_t item_count = 0;
  char *xmldata = NULL;
  uint32_t fileLen = 0;
  uint32_t dummy_ttl;
  simple_list items;

  assert(xmlfile && *xmlfile);
  dbg_printf(P_INFO, "Reading RSS feed file: %s", xmlfile);
  xmldata = readFile(xmlfile, &fileLen);

  if(xmldata != NULL) {
    fileLen = strlen(xmldata);
    items = parse_xmldata(xmldata, fileLen, &item_count, &dummy_ttl);
    session->max_bucket_items += item_count;
    dbg_printf(P_INFO2, "History bucket size changed: %d", session->max_bucket_items);
    processRSSList(session, NULL, items, NULL);
    freeList(&items, freeFeedItem);
    am_free(xmldata);
  }

  return item_count;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
  auto_handle * ses = NULL;
  char *config_file = NULL;
  char *logfile = NULL;
  char *pidfile = NULL;
  char *xmlfile = NULL;
  char erbuf[100];
  NODE *current = NULL;
  uint32_t count = 0;
  uint8_t first_run = 1;
  uint8_t once = 0;
  uint8_t verbose = AM_DEFAULT_VERBOSE;
  uint8_t append_log = 0;
  uint8_t match_only = 0;

  /* this sets the log level to the default before anything else is done.
  ** This way, if any outputting happens in readargs(), it'll be printed
  ** to stderr.
  */
  log_init(NULL, verbose, 0);

  readargs(argc, argv, &config_file, &logfile, &pidfile, &xmlfile, &nofork, &verbose, &once, &append_log, &match_only);

  /* reinitialize the logging with the values from the command line */
  log_init(logfile, verbose, append_log);

  dbg_printf(P_MSG, "Automatic version: %s", LONG_VERSION_STRING);

  if(!config_file) {
    config_file = am_strdup(AM_DEFAULT_CONFIGFILE);
  }

  strncpy(AutoConfigFile, config_file, strlen(config_file));

  ses = session_init();
  ses->match_only = match_only;

  if(parse_config_file(ses, AutoConfigFile) != 0) {
     if(errno == ENOENT) {
        snprintf(erbuf, sizeof(erbuf), "Cannot find file '%s'", config_file);
     } else {
        snprintf(erbuf, sizeof(erbuf), "Unknown error");
     }

     fprintf(stderr, "Error parsing config file: %s\n", erbuf);
     shutdown_daemon(ses);
  }

  if(!setupSession(ses)) {
     shutdown_daemon(ses);
  }

  setup_signals();

  if(!nofork) {
    /* start daemon */
    if(daemonize(pidfile) != 0) {
      dbg_printf(P_ERROR, "Error: Daemonize failed. Aborting...");
      shutdown_daemon(mySession);
    }
    dbg_printft( P_MSG, "Daemon started");
  }

  dbg_printf(P_INFO, "verbose level: %d", verbose);
  dbg_printf(P_INFO, "foreground mode: %s", nofork == true ? "yes" : "no");

  while(!closing) {
    isRunning = true;
    dbg_printft( P_INFO, "------ Checking for new episodes ------");
    if(xmlfile && *xmlfile) {
      processFile(mySession, xmlfile);
      once = 1;
    } else {
      current = mySession->feeds;
      count = 0;
      while(current && current->data) {
        ++count;
        processFeed(mySession, current->data, first_run);
        current = current->next;
      }
      if(first_run) {
        dbg_printf(P_INFO2, "New bucket size: %d", mySession->max_bucket_items);
      }
      first_run = 0;
    }

    /* leave loop when program is only supposed to run once */
    if(once) {
      break;
    }

    isRunning = false;

    if(seenHUP) {
      signal_handler(SIGHUP);
    }

    sleep(mySession->check_interval * 60);
  }

  shutdown_daemon(mySession);
  return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PRIVATE bool isMagnetURI(const char* url) {
  if(url == NULL || *url == 0) {
    return false;
  }

  if(strlen(url) < 7) {
    return false;
  }

  return !strncmp(url, "magnet:", 7);

}

