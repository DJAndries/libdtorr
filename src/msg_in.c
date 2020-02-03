#include "msg_in.h"
#include "log.h"
#include "piece_ex.h"
#include "util.h"

#define MSG_CHOKE 0
#define MSG_UNCHOKE 1
#define MSG_INTERESTED 2
#define MSG_NOT_INTERESTED 3
#define MSG_HAVE 4
#define MSG_BITFIELD 5
#define MSG_REQUEST 6
#define MSG_PIECE 7
#define MSG_CANCEL 8

static int handle_piece(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
  char* in, unsigned long in_len) {

  unsigned int index;
  unsigned int begin;

  if (in_len < 10 || in_len > 20000) {
    dlog(config, LOG_LEVEL_WARN, "Received piece with message that is too small or too big");
    return 1;
  }
  
  index = bigend_to_uint(in + 1);
  begin = bigend_to_uint(in + 5);

  if ((begin + (in_len - 9)) > torrent->piece_length) {
    dlog(config, LOG_LEVEL_WARN, "Received piece that is longer than piece length");
    return 2;
  }

  return piece_recv(config, torrent, peer, index, begin, in + 9, in_len - 9);
}

static int handle_bitfield(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
  char* in, unsigned long in_len) {

  unsigned long i, j;
  unsigned long bitfield_exp_length = (torrent->piece_count / 8) + (torrent->piece_count % 8 != 0);
  if ((in_len - 1) < bitfield_exp_length) {
    dlog(config, LOG_LEVEL_WARN, "Bad bitfield length");
    return 1;
  }

  for (i = 0; i < in_len - 1; i++) {
    for (j = 0; j < 8 && (i * 8 + j) < torrent->piece_count; j++) {
      peer->bitfield[i * 8 + j] = (in[i + 1] >> (7 - j)) & 0x01;
    }
  }
  return 0;
}

int process_msg(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
  char* in, unsigned long in_len, char* out, unsigned long* out_len) {
  
  *out_len = 0;
  if (in_len == 0) {
    /* set keep alive time */
    return 0;
  }

  switch (in[0]) {
    case MSG_CHOKE:
    case MSG_UNCHOKE:
      dlog(config, LOG_LEVEL_DEBUG, "Peer choke status changed: %d", in[0] == MSG_CHOKE);
      peer->choked = in[0] == MSG_CHOKE;
      break;
    case MSG_INTERESTED:
    case MSG_NOT_INTERESTED:
      dlog(config, LOG_LEVEL_DEBUG, "Peer interested status changed: ", in[0] == MSG_INTERESTED);
      peer->they_interested = in[0] == MSG_INTERESTED;
      break;
    case MSG_HAVE:
      break;
    case MSG_BITFIELD:
      dlog(config, LOG_LEVEL_DEBUG, "Received bitfield");
      return handle_bitfield(config, torrent, peer, in, in_len);
    case MSG_REQUEST:
      /* return handle_request(config, torrent, peer, in, in_len); */
    case MSG_PIECE:
      return handle_piece(config, torrent, peer, in, in_len);
    case MSG_CANCEL:
      break;
  }

  return 0;
}