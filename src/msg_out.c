#include "msg_out.h"
#include "msg.h"
#include "stream.h"
#include "log.h"
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
  return 0;
}