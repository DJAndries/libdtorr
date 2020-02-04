#include "handshake.h"
#include "dsock.h"
#include "list.h"
#include "log.h"
#include <string.h>

#define BT_HEADER "\x13" "BitTorrent protocol\x00\x00\x00\x00\x00\x00\x00\x00"
#define BT_HEADER_LEN 28
#define BT_HS_LEN 48

int peer_handshake(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer) {
  char msg[128];
  peer->s = dsock_connect(peer->ip, peer->port);
  if (peer->s == INVALID_SOCKET) {
    dlog(config, LOG_LEVEL_INFO, "Unable to connect to peer %s:%u", peer->ip, peer->port);
    return 1;
  }
  dlog(config, LOG_LEVEL_INFO, "Connected to peer %s:%u", peer->ip, peer->port);

  memcpy(msg, BT_HEADER, BT_HEADER_LEN);
  memcpy(msg + BT_HEADER_LEN, torrent->infohash, 20);
  memcpy(msg + BT_HEADER_LEN + 20, torrent->me.peer_id, 20);

  if (peer->bitfield == 0) {
    peer->bitfield = (char*)malloc(sizeof(char) * torrent->piece_count);
    if (peer->bitfield == 0) {
      dlog(config, LOG_LEVEL_ERROR, "Failed to alloc bitfield for peer");
      peer->s = dsock_close(peer->s);
      return 2;
    }
    memset(peer->bitfield, 0, sizeof(char) * torrent->piece_count);
  }

  if (send(peer->s, msg, BT_HS_LEN, 0) != BT_HS_LEN) {
    dlog(config, LOG_LEVEL_ERROR, "Failed to send header to peer");
    peer->s = dsock_close(peer->s);
    return 3;
  }
  
  if (recv(peer->s, msg, BT_HS_LEN, MSG_WAITALL) != BT_HS_LEN) {
    dlog(config, LOG_LEVEL_ERROR, "Invalid recv header size");
    peer->s = dsock_close(peer->s);
    return 4;
  }

  if (memcmp(msg, BT_HEADER, 20) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Invalid BT header");
    peer->s = dsock_close(peer->s);
    return 5;
  }

  if (memcmp(msg + BT_HEADER_LEN, torrent->infohash, 20) != 0) {
    dlog(config, LOG_LEVEL_WARN, "Peer did not return same infohash. Closing conn");
    peer->s = dsock_close(peer->s);
    return 6;
  }

  memcpy(peer->peer_id, msg + BT_HEADER_LEN + 20, 20);

  dlog(config, LOG_LEVEL_INFO, "Handshake success %s:%u", peer->ip, peer->port);
  if (list_insert(&torrent->active_peers, peer) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Fail to add peer to active list");
    peer->s = dsock_close(peer->s);
  }
  peer->active = 1;
  torrent->active_peer_count++;
  return 0;
}