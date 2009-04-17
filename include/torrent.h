/*
 * torrent.h
 *
 *  Created on: Oct 13, 2008
 *      Author: aurich
 */

#ifndef TORRENT_H_
#define TORRENT_H_

void get_filename(char *filename, const char *content_filename, const char *url, const char *tm_path);
int8_t uploadTorrent(const void *t_data, int t_size,
                     const char *host, const char* auth, uint16_t port,
                     int16_t upspeed, uint8_t start);

#endif /* TORRENT_H_ */
