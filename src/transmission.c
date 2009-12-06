/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file transmission.c
 *
 * Transmission-specific functionality.
 *
 * \internal Created on: Oct 13, 2008
 * Author: Frank Aurich
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>

#include "output.h"
#include "web.h"
#include "json.h"
#include "utils.h"

/** \brief (Deprecated) Call Transmission via system() call to add a torrent
 *
 * \param tm_path Full path to the Transmission config folder
 * \param filename Full path to the new torrent
 * \return 0 if the torrent was successfully added, -1 otherwise.
 *
 * This function is used with older versions of Transmission (1.2x) where the JSON-RPC framework
 * was not in place yet.
 * It remains for compatibility reasons.
 */
int8_t call_transmission(const char* tm_path, const char *filename) {
	char buf[PATH_MAX];
	int8_t status;

	if(filename && strlen(filename) > 1) {
		snprintf(buf, PATH_MAX, "transmission-remote -g \"%s\" -a \"%s\"", tm_path, filename);
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

/** \brief Call external program via system() call to add a torrent
 *
 * \param external_name Full path to program to call
 * \param filename Full path to the new torrent
 * \return -1 for an internal error, or external program's return value
 * (probably 0 for success or non-0 for failure)
 *
 * This function is used when transmission-version is set to "external".
 * external_name is populated from the value of transmission-external.
 * This is used when using a client other than transmission or using
 * transmission with special needs, like external accounting or directory
 * setup.
 */
int8_t call_external(const char *external_name, const char *filename) {
  char buf[PATH_MAX];
  int8_t status;

  if ( (external_name == NULL) || (filename == NULL) || (strlen(filename) < 1) )
  {
    dbg_printf(P_ERROR, "[call_external] Error: invalid parameter\n");
    return -1;
  }

  snprintf(buf, PATH_MAX, "%s %s", external_name, filename);
  dbg_printf(P_INFO2, "[call_external] Calling %s", buf);
  /* system needs SIGCHLD set to default */
  signal(SIGCHLD, SIG_DFL);
  /* note: automatic will ignore SIGTERM, SIGINT, etc. while waiting for the
    system() call to complete */
  status = system(buf);
  if( status == -1 ) {
    dbg_printf(P_ERROR, "[call_external] system failed: %s", strerror(errno));
    /* could call setup_signals() from automatic.c here */
    signal(SIGCHLD, SIG_IGN);
    return -1;
  } else {
    dbg_printf(WEXITSTATUS(status) != 0 ? P_ERROR : P_INFO2,
               "[call_external] External exit status %d",
               WEXITSTATUS(status));
    /* could call setup_signals() from automatic.c here */
    signal(SIGCHLD, SIG_IGN);
    return WEXITSTATUS(status);
  }
  /*NOTREACHED*/
}

int8_t getRPCVersion(const char* host, uint16_t port, const char* auth) {

	char url[MAX_URL_LEN];
	int8_t result = 0;
	char *response = NULL;
	HTTPResponse *res = NULL;
	const char *JSONstr =
	 "{\n"
	 "\"method\": \"session-get\",\n"
	 "\"arguments\": {}\n"
	 "}";

	if(!host) {
		return 0;
	}

	snprintf( url, MAX_URL_LEN, "http://%s:%d/transmission/rpc", host, port);

  res = sendHTTPData(url, auth, JSONstr, strlen(JSONstr));
	if(res != NULL && res->responseCode == 200) {
	  dbg_printf(P_DBG, "[getRPCVersion] got response!");
		response = parseResponse(res->data);
		if(response) {
			if(!strncmp(response, "success", 7)) {
				result = parseRPCVersion(res->data);
        dbg_printf(P_DBG, "[getRPCVersion] RPC version: %d", result);
			}
			am_free((void*)response);
		}
		HTTPResponse_free(res);
	}
	return result;
}




