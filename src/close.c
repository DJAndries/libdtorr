#include "close.h"
#include "dsock.h"
#include "list.h"
#include <string.h>

void peer_close(dtorr_torrent* torrent, dtorr_peer* peer) {
  dtorr_listnode* it;
  if (peer->s != INVALID_SOCKET) {
    peer->s = dsock_close(peer->s);
  }
  peer->active = 0;
  peer->out_request_count = 0;
  memset(peer->peer_id, 0, 20);
  
  if (peer->bitfield != 0) {
    free(peer->bitfield);
    peer->bitfield = 0;
  }

  for (it = peer->out_piece_requests; it != 0; it = it->next) {
    torrent->out_piece_request_peer_map[((dtorr_piece_request*)it->value)->index] = 0;
  }  
  list_free(peer->out_piece_requests, 1);
  peer->out_piece_requests = 0;

  list_remove(&torrent->active_peers, peer);
}