#include "piece_ex.h"
#include "fs.h"
#include "msg_out.h"
#include "list.h"
#include <stdlib.h>

static int piece_completed(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer, unsigned long index) {
  torrent->bitfield[index] = 1;
  torrent->out_piece_request_peer_map[index] = 0;
  /* verify piece, return bad return value if bad */
  return send_have(config, torrent, index);
}

int piece_recv(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
  unsigned long index, unsigned long begin, char* piece, unsigned long piece_len) {
  dtorr_listnode *it, *next;
  dtorr_piece_request *it_req;
  char is_request_left = 0;

  if (save_piece(config, torrent, index, begin, piece, piece_len) != 0) {
    return 3;
  }

  for (it = peer->out_piece_requests; it != 0; it = next) {
    next = it->next;
    it_req = (dtorr_piece_request*)it->value;
    if (it_req->index == index) {
      if (it_req->begin == begin) {
        list_remove(&peer->out_piece_requests, it_req);
        peer->sent_request_count--;
        peer->total_request_count--;
        free(it_req);
      } else {
        is_request_left = 1;
      }
    }
  }

  if (is_request_left == 0) {
    return piece_completed(config, torrent, peer, index);
  }

  return 0;
}
