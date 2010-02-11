/*
 * torrent.h
 *
 *  Created on: Oct 13, 2008
 *      Author: aurich
 */

#ifndef TORRENT_H_
#define TORRENT_H_

typedef uint32_t torrent_id_t;

void get_filename(char *filename, const char *content_filename, const char *url, const char *tm_path);
torrent_id_t uploadTorrent(const void *t_data, int t_size,
                           const char *url, const char* auth, uint8_t start);

int8_t changeUploadSpeed(const char* url, const char* auth, torrent_id_t id,
                         uint16_t upspeed, uint8_t rpcVersion);

#endif /* TORRENT_H_ */
