#include "choke.h"
#include <stdlib.h>
#include "util.h"
#include "msg_out.h"
#include "close.h"
#include "log.h"

#define min(x, y) (x < y ? x : y)
#define MAX_RAND_THRESHOLD 27000
#define MAX_UPLOAD_PEERS 10

static int choke(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer) {
  dtorr_listnode *it, *next;

  peer->we_choked = 1;

  for (it = peer->in_piece_requests; it != 0; it = next) {
    next = it->next;
    free(it->value);
    free(it);
  }
  peer->in_piece_requests = 0;

  peer->total_in_request_count = 0;
  if (peer->curr_in_piece != 0) {
    free(peer->curr_in_piece);
    peer->curr_in_piece = 0;
  }
  if (send_choked_status(config, peer, 1) != 0) {
    peer_close(config, torrent, peer, 0);
    return 1;
  }
  return 0;
}

void choke_update(dtorr_config* config, dtorr_torrent* torrent) {
  dtorr_listnode *it, *next;
  dtorr_peer* peer;
  unsigned long interested_count = 0;
  unsigned long unchoked_count = 0;
  long unchoke_threshold;
  char should_choke;

  for (it = torrent->active_peers; it != 0; it = next) {
    next = it->next;
    peer = (dtorr_peer*)it->value;
    if (peer->they_interested == 0 && peer->we_choked == 0) {
      dlog(config, LOG_LEVEL_DEBUG, "Choking peer because they are not interested");
      if (choke(config, torrent, peer) != 0) {
        continue;
      }
    }
    if (peer->they_interested == 1) {
      interested_count++;
    }
  }

  srand((unsigned int)get_time_ms());
  unchoke_threshold = (long)(MAX_RAND_THRESHOLD * ((double)min(MAX_UPLOAD_PEERS, interested_count) / MAX_UPLOAD_PEERS));

  dlog(config, LOG_LEVEL_DEBUG, "Unchoke threshold: %d", unchoke_threshold);

  for (it = torrent->active_peers; it != 0 && unchoked_count < MAX_UPLOAD_PEERS; it = next) {
    next = it->next;
    peer = (dtorr_peer*)it->value;
    should_choke = rand() < unchoke_threshold;
    if (peer->they_interested == 1 && peer->we_choked == 1 && should_choke == 0) {
      dlog(config, LOG_LEVEL_DEBUG, "Unchoking peer because they are interested");
      peer->we_choked = 0;
      unchoked_count++;
      if (send_choked_status(config, peer, 0) != 0) {
        peer_close(config, torrent, peer, 0);
      }
    } else if (peer->we_choked == 0 && should_choke == 1) {
      dlog(config, LOG_LEVEL_DEBUG, "Choking peer because did not meet choke rand threshold");
      choke(config, torrent, peer);
    }
  }
}

