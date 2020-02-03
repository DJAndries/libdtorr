#include "piece_ex.h"
#include "fs.h"

static int piece_completed(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer, unsigned long index) {

  /* verify piece, return bad return value if bad */
  return 0;
}

int piece_recv(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
  unsigned long index, unsigned long begin, char* piece, unsigned long piece_len) {
  dtorr_listnode* it;
  char is_request_left = 0;

  if (save_piece(config, torrent, index, begin, piece, piece_len) != 0) {
    return 3;
  }

  for (it = peer->out_piece_requests; it != 0; it = it->next) {
    if (((dtorr_piece_request*)it->value)->index == index) {
      is_request_left = 1;
    }
  }

  if (is_request_left == 0) {
    return piece_completed(config, torrent, peer, index);
  }

  return 0;
}
