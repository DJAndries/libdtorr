#include "msg_in.h"
#include <stdlib.h>
#include "msg.h"
#include "log.h"
#include "piece_ex.h"
#include "util.h"

static int handle_piece(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
  char* in, unsigned long long in_len) {

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

static int handle_request(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
                          char* in, unsigned long long in_len) {
  unsigned int index, begin, length;
  unsigned long long piece_length;

  if (in_len != 13) {
    dlog(config, LOG_LEVEL_WARN, "Message too small or too big for message request");
    return 1;
  }

  if (peer->we_choked == 1) {
    /* ignore */
    dlog(config, LOG_LEVEL_WARN, "Received request when we choked");
    return 2;
  }

  index = bigend_to_uint(in + 1);
  begin = bigend_to_uint(in + 5);
  length = bigend_to_uint(in + 9);

  if (index >= torrent->piece_count) {
    dlog(config, LOG_LEVEL_WARN, "Requested piece index exceeds actual piece count");
    return 3;
  }

  piece_length = calc_piece_length(torrent->piece_count, torrent->piece_length,
                                   torrent->length, index);

  if ((begin + length) > piece_length) {
    dlog(config, LOG_LEVEL_WARN, "Requested piece exceeds actual piece length");
    return 4;
  }

  dlog(config, LOG_LEVEL_DEBUG, "Received piece request for index %lu begin %lu", index, begin);
  if (queue_request(config, peer, index, begin, length) != 0) {
    return 5;
  }

  return 0;
}

static int handle_bitfield(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
  char* in, unsigned long long in_len) {

  unsigned long long i, j;
  unsigned long long bitfield_exp_length = (torrent->piece_count / 8) + (torrent->piece_count % 8 != 0);
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

static int handle_have(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer, char* in, unsigned long long in_len) {
  unsigned long long index;
  
  if (in_len < 5) {
    dlog(config, LOG_LEVEL_WARN, "Bad have length");
    return 1;
  }

  index = bigend_to_uint(in + 1);

  if (index >= torrent->piece_count) {
    dlog(config, LOG_LEVEL_WARN, "Have index exceeds piece count");
    return 2;
  }

  peer->bitfield[index] = 1;

  return 0;
}

static void handle_choke(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer, char choked) {
  dtorr_listnode *it, *next;
  unsigned long long index;
  dlog(config, LOG_LEVEL_DEBUG, "Peer choke status changed: %d", choked);
  peer->they_choked = choked;
  if (choked == 0) {
    return;
  }
  for (it = peer->out_piece_requests; it != 0; it = next) {
    next = it->next;
    index = ((dtorr_piece_request*)it->value)->index;
    if (torrent->in_piece_buf_map[index] != 0) {
      free(torrent->in_piece_buf_map[index]);
      torrent->in_piece_buf_map[index] = 0;
    }
    free(it->value);
    free(it);
  }
  peer->out_piece_requests = 0;
  peer->total_out_request_count = peer->sent_request_count = 0;
}

int process_msg(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer, char* in, unsigned long long in_len) {
  
  if (in_len == 0) {
    /* set keep alive time */
    return 0;
  }

  switch (in[0]) {
    case MSG_CHOKE:
    case MSG_UNCHOKE:
      handle_choke(config, torrent, peer, in[0] == MSG_CHOKE);
      break;
    case MSG_INTERESTED:
    case MSG_NOT_INTERESTED:
      dlog(config, LOG_LEVEL_DEBUG, "Peer interested status changed: %d", in[0] == MSG_INTERESTED);
      peer->they_interested = in[0] == MSG_INTERESTED;
      break;
    case MSG_HAVE:
      return handle_have(config, torrent, peer, in, in_len);
    case MSG_BITFIELD:
      dlog(config, LOG_LEVEL_DEBUG, "Received bitfield");
      return handle_bitfield(config, torrent, peer, in, in_len);
    case MSG_REQUEST:
      return handle_request(config, torrent, peer, in, in_len);
      break;
    case MSG_PIECE:
      return handle_piece(config, torrent, peer, in, in_len);
    case MSG_CANCEL:
      break;
  }

  return 0;
}
