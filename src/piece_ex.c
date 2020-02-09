#include "piece_ex.h"
#include "fs.h"
#include "msg_out.h"
#include "list.h"
#include "log.h"
#include "util.h"
#include <stdlib.h>

static int piece_completed(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer, unsigned long index) {
  unsigned long piece_length = calc_piece_length(torrent->piece_count, torrent->piece_length, torrent->length, index);

  torrent->bitfield[index] = 1;

  if (save_piece(config, torrent, index, torrent->out_piece_buf_map[index], piece_length) != 0) {
    return 1;
  }
  free(torrent->out_piece_buf_map[index]);
  torrent->out_piece_buf_map[index] = 0;
  /* verify piece, return bad return value if bad */
  return send_have(config, torrent, index);
}

int piece_recv(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
  unsigned long index, unsigned long begin, char* piece, unsigned long piece_len) {
  dtorr_listnode *it, *next;
  dtorr_piece_request *it_req;
  char is_request_left = 0;

  dlog(config, LOG_LEVEL_DEBUG, "Recv piece part index: %lu begin: %lu", index, begin);

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
