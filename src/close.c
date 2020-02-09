#include "close.h"
#include "dsock.h"
#include "list.h"
#include "log.h"
#include <string.h>

void peer_close(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer, char bad) {
  dlog(config, LOG_LEVEL_INFO, "Closing peer %s:%u. Bad: %d", peer->ip, peer->port, bad);
  dtorr_listnode* it;
  unsigned long index;
  if (peer->s != INVALID_SOCKET) {
    peer->s = dsock_close(peer->s);
  }
  torrent->active_peer_count--;
  peer->active = 0;
  peer->bad = bad;

  peer->we_choked = peer->they_choked = 1;
  peer->they_interested = peer->we_interested = 0;

  peer->total_request_count = peer->sent_request_count = 0;
  memset(peer->peer_id, 0, 20);
  
  if (peer->bitfield != 0) {
    free(peer->bitfield);
    peer->bitfield = 0;
  }

  for (it = peer->out_piece_requests; it != 0; it = it->next) {
    index = ((dtorr_piece_request*)it->value)->index;
    if (torrent->out_piece_buf_map[index] != 0) {
      free(torrent->out_piece_buf_map[index]);
      torrent->out_piece_buf_map[index] = 0;
    }
  }  
  list_free(peer->out_piece_requests, 1);
  peer->out_piece_requests = 0;

  list_remove(&torrent->active_peers, peer);
}