#include "msg_out.h"
#include "msg.h"
#include "stream.h"
#include "close.h"
#include "log.h"
#include "util.h"
#include <string.h>

int send_bitfield(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer) {
  unsigned long i;
  unsigned long compact_len = (torrent->piece_count / 8) + (torrent->piece_count % 8 != 0);
  char* buf = (char*)malloc(sizeof(char) * compact_len + 1);

  if (buf == 0) {
    return 1;
  }

  buf[0] = MSG_BITFIELD;
  memset(buf + 1, 0, compact_len);

  for (i = 0; i < torrent->piece_count; i++) {
    buf[i / 8 + 1] |= (torrent->bitfield[i] << (7 - (i % 8)));
  }

  if (send_sock_msg(peer->s, buf, compact_len + 1) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Failed to send bitfield");
    return 2;
  }
  free(buf);
  return 0;
}

int send_interested_status(dtorr_config* config, dtorr_peer* peer, char interested) {
  char buf = interested == 1 ? 2 : 3;

  if (send_sock_msg(peer->s, &buf, 1) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Failed to send interested update");
    return 1;
  }
  return 0;
}

int send_have(dtorr_config* config, dtorr_torrent* torrent, unsigned long index) {
  char buf[5];
  dtorr_listnode *it, *next;
  dtorr_peer* peer;

  buf[0] = 4;
  uint_to_bigend(buf + 1, index);

  for (it = torrent->active_peers; it != 0; it = next) {
    next = it->next;
    peer = (dtorr_peer*)it->value;
    if (send_sock_msg(peer->s, buf, 5) != 0) {
      dlog(config, LOG_LEVEL_ERROR, "Failed to send have update");
      peer_close(config, torrent, peer, 0);
    }
  }
  return 0;
}

int send_request(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer, dtorr_piece_request* req) {
  char buf[13];

  buf[0] = 6;
  uint_to_bigend(buf + 1, (unsigned int)req->index);
  uint_to_bigend(buf + 5, (unsigned int)req->begin);
  uint_to_bigend(buf + 9, (unsigned int)req->length);

  if (send_sock_msg(peer->s, buf, 13) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Failed to send piece request");
    peer_close(config, torrent, peer, 0);
  }

  return 0;
}