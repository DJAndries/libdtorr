#include "handshake.h"
#include "dsock.h"
#include "list.h"
#include "log.h"
#include "peer.h"
#include "msg_out.h"
#include "close.h"
#include <stdio.h>
#include <string.h>

#define BT_HEADER "\x13" "BitTorrent protocol\x00\x00\x00\x00\x00\x00\x00\x00"
#define BT_HEADER_LEN 28
#define BT_HS_LEN 68

static int send_header(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer) {
  char msg[128];

  memcpy(msg, BT_HEADER, BT_HEADER_LEN);
  memcpy(msg + BT_HEADER_LEN, torrent->infohash, 20);
  memcpy(msg + BT_HEADER_LEN + 20, torrent->me.peer_id, 20);

  if (send(peer->s, msg, BT_HS_LEN, 0) != BT_HS_LEN) {
    dlog(config, LOG_LEVEL_ERROR, "Failed to send header to peer");
    return 1;
  }

  return 0;
}

static int recv_header(dtorr_config* config, char* msg, SOCKET s) {
  if (recv(s, msg, BT_HS_LEN, MSG_WAITALL) != BT_HS_LEN) {
    dlog(config, LOG_LEVEL_ERROR, "Invalid recv header size");
    return 1;
  }

  if (memcmp(msg, BT_HEADER, 20) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Invalid BT header");
    return 2;
  }

  return 0;
}

static int check_infohash(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer, char* recv_infohash) {
  if (memcmp(recv_infohash, torrent->infohash, 20) != 0) {
    dlog(config, LOG_LEVEL_WARN, "Peer did not return same infohash. Closing conn");
    return 1;
  }

  return 0;
}

static int insert_active_peer(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer) {
  if (list_insert(&torrent->active_peers, peer) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Fail to add peer to active list");
    return 1;
  }
  if (peer->bitfield == 0) {
    if (peer_init_bitfield(config, torrent, peer) != 0) {
      peer->s = dsock_close(peer->s);
      return 2;
    }
  }
  peer->active = 1;
  peer->they_choked = peer->we_choked = 1;
  torrent->active_peer_count++;
  return 0;
}

int peer_send_handshake(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer) {
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

  if (send_header(config, torrent, peer) != 0) {
    peer->s = dsock_close(peer->s);
    return 3;
  }

  if (recv_header(config, msg, peer->s) != 0) {
    peer->s = dsock_close(peer->s);
    return 4;
  }

  if (check_infohash(config, torrent, peer, msg + BT_HEADER_LEN) != 0) {
    peer->s = dsock_close(peer->s);
    return 5;
  }

  memcpy(peer->peer_id, msg + BT_HEADER_LEN + 20, 20);

  if (dsock_set_sock_nonblocking(peer->s) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Fail to set socket as non blocking");
    peer->s = dsock_close(peer->s);
    return 6;
  }

  if (insert_active_peer(config, torrent, peer) != 0) {
    peer->s = dsock_close(peer->s);
    return 7;
  }

  if (send_bitfield(config, torrent, peer) != 0) {
    peer_close(config, torrent, peer, 0);
    return 8;
  }

  dlog(config, LOG_LEVEL_INFO, "Handshake success (send) %s:%u", peer->ip, peer->port);

  return 0;
}

int peer_recv_handshake(dtorr_config* config, SOCKET s, char* ip, unsigned short port) {
  char msg[128];

  dtorr_peer* peer;
  dtorr_torrent* torrent;

  unsigned long mode = 0;
  ioctlsocket(s, FIONBIO, &mode);

  if (recv_header(config, msg, s) != 0) {
    dsock_close(s);
    return 1;
  }

  torrent = config->torrent_lookup(msg + BT_HEADER_LEN);
  if (torrent == 0) {
    dsock_close(s);
    return 2;
  }

  peer = peer_get_or_create(config, torrent, ip, port);
  if (peer == 0 || peer->active == 1) {
    dsock_close(s);
    return 3;
  }
  memcpy(peer->peer_id, msg + BT_HEADER_LEN + 20, 20);

  peer->s = s;

  if (send_header(config, torrent, peer) != 0) {
    dsock_close(s);
    return 4;
  }

  if (dsock_set_sock_nonblocking(s) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Fail to set socket as non blocking");
    dsock_close(s);
    return 5;
  }

  if (insert_active_peer(config, torrent, peer) != 0) {
    dsock_close(s);
    return 6;
  }

  if (send_bitfield(config, torrent, peer) != 0) {
    peer_close(config, torrent, peer, 0);
    return 7;
  }

  dlog(config, LOG_LEVEL_INFO, "Handshake success (recv) %s:%u", peer->ip, peer->port);

  return 0;
}
