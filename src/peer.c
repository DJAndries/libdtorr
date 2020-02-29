#include "peer.h"
#include "hashmap.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

dtorr_peer* peer_get_or_create(dtorr_config* config, dtorr_torrent* torrent, char* ip, unsigned short port) {
  char ip_and_port[128];

  sprintf(ip_and_port, "%s:%u", ip, port);

  dtorr_peer* peer = (dtorr_peer*)hashmap_get(torrent->peer_map, ip_and_port);

  if (peer != 0) {
    return peer;
  }

  peer = (dtorr_peer*)malloc(sizeof(dtorr_peer));
  if (peer == 0) {
    dlog(config, LOG_LEVEL_ERROR, "failed to allocate peer");
    return 0;
  }

  if (hashmap_insert(torrent->peer_map, ip_and_port, peer) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "failed to insert peer into map");
    free(peer);
    return 0;
  }

  memset(peer, 0, sizeof(dtorr_peer));
  strcpy(peer->ip, ip);
  peer->port = port;

  return peer;
}

int peer_init_bitfield(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer *peer) {
  peer->bitfield = (char*)malloc(sizeof(char) * torrent->piece_count);
  if (peer->bitfield == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Failed to alloc bitfield for peer");
    return 1;
  }
  memset(peer->bitfield, 0, sizeof(char) * torrent->piece_count);
  return 0;
}
