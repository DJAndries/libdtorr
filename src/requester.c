#include "requester.h"
#include "log.h"
#include "close.h"
#include "msg_out.h"
#include "util.h"

void interest_update(dtorr_config* config, dtorr_torrent* torrent) {
  long interest_index;
  dtorr_listnode *it, *next;
  dtorr_peer* peer;
  char interested;
  for (it = torrent->active_peers; it != 0; it = next) {
    peer = (dtorr_peer*)it->value;
    next = it->next;
    interested = -1;

    interest_index = bitfield_interest_index(torrent->bitfield, peer->bitfield, torrent->piece_count, 0);

    if (interest_index == -1 && peer->we_interested == 1) {
      interested = 0;
    } else if (interest_index != -1 && peer->we_interested == 0) {
      interested = 1;
    }

    if (interested != -1) {
      peer->we_interested = interested;
      if (send_interested_status(config, torrent, peer, interested) != 0) {
        peer_close(config, torrent, peer, 0);
        continue;
      }
      dlog(config, LOG_LEVEL_DEBUG, "Sent interested status: %d", interested);
    }
  }
}

int request_pieces(dtorr_config* config, dtorr_torrent* torrent) {
  return 0;
}