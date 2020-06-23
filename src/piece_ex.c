#include "piece_ex.h"
#include "dtorr/fs.h"
#include "msg_out.h"
#include "list.h"
#include "log.h"
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>

#define MAX_IN_REQS 1024

static int verify_hash(dtorr_config* config, dtorr_torrent* torrent, char* buf, unsigned long long index, unsigned long long len) {
  unsigned char* hash;
  if ((hash = SHA1((unsigned char*)buf, len, 0)) == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Unable to hash/verify piece");
    return 1;
  }

  if (memcmp(hash, torrent->pieces + (20 * index), 20) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Hash mismatch for piece index %lu", index);
    return 2;
  }
  return 0;
}

static int piece_completed(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer, unsigned long long index) {
  unsigned long long piece_length = calc_piece_length(torrent->piece_count, torrent->piece_length, torrent->length, index);

  if (verify_hash(config, torrent, torrent->in_piece_buf_map[index], index, piece_length) != 0) {
    return 1;
  }

  if (rw_piece(config, torrent, index, torrent->in_piece_buf_map[index], piece_length, 1) != 0) {
    return 3;
  }

  torrent->bitfield[index] = 1;
  torrent->downloaded += piece_length;
  torrent->downloaded_interval += piece_length;

  if (torrent->in_piece_buf_map[index] != 0) {
    free(torrent->in_piece_buf_map[index]);
    torrent->in_piece_buf_map[index] = 0;
  }

  return send_have(config, torrent, index);
}

int piece_recv(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer,
  unsigned long long index, unsigned long long begin, char* piece, unsigned long long piece_len) {
  dtorr_listnode *it, *next;
  dtorr_piece_request *it_req;
  char is_request_left = 0;

  dlog(config, LOG_LEVEL_DEBUG, "Recv piece part index: " SPEC_LLU " begin: " SPEC_LLU, index, begin);

  if (torrent->in_piece_buf_map[index] == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Recv piece that was not requested");
    return 1;
  }

  memcpy(torrent->in_piece_buf_map[index] + begin, piece, piece_len);

  for (it = peer->out_piece_requests; it != 0; it = next) {
    next = it->next;
    it_req = (dtorr_piece_request*)it->value;
    if (it_req->index == index) {
      if (it_req->begin == begin) {
        list_remove(&peer->out_piece_requests, it_req);
        peer->sent_request_count--;
        peer->total_out_request_count--;
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

static int load_piece_data(dtorr_config* config, dtorr_torrent* torrent,
                                     dtorr_peer* peer, unsigned long long index) {
  unsigned long long len = calc_piece_length(torrent->piece_count, torrent->piece_length, torrent->length, index);
  peer->curr_in_piece = (char*)malloc(sizeof(char) * len);
  if (peer->curr_in_piece == 0) {
    return 1;
  }
  if (rw_piece(config, torrent, index, peer->curr_in_piece, len, 0) != 0) {
    return 2;
  }
  if (verify_hash(config, torrent, peer->curr_in_piece, index, len) != 0) {
    return 3;
  }
  
  return 0;
}

int queue_request(dtorr_config* config, dtorr_peer* peer, unsigned long long index,
                  unsigned long long begin, unsigned long long length) {
  dtorr_piece_request* req;

  req = (dtorr_piece_request*)malloc(sizeof(dtorr_piece_request));
  if (req == 0) {
    return 1;
  }

  req->index = index;
  req->begin = begin;
  req->length = length;

  if (list_insert(&peer->in_piece_requests, req) != 0) {
    return 2;
  }

  peer->total_in_request_count++;
  return 0;
}

int pieces_send(dtorr_config* config, dtorr_peer* peer, dtorr_torrent* torrent) {
  dtorr_listnode *piece_node;
  dtorr_piece_request* req;

  if (peer->in_piece_requests == 0) {
    return 0;
  }
  piece_node = peer->in_piece_requests;
  req = (dtorr_piece_request*)piece_node->value;

  if (peer->curr_in_piece == 0 || peer->curr_in_piece_index != req->index) {
    if (peer->curr_in_piece != 0) {
      free(peer->curr_in_piece);
      peer->curr_in_piece = 0;
    }
    if (load_piece_data(config, torrent, peer, req->index) != 0) {
      return -1;
    }
    peer->curr_in_piece_index = req->index;
  }

  if (send_piece(config, torrent, peer, req, peer->curr_in_piece) != 0) {
    return 2;
  }

  torrent->uploaded += req->length;
  torrent->uploaded_interval += req->length;

  peer->in_piece_requests = piece_node->next;
  free(piece_node);
  peer->total_in_request_count--;

  return 0;
}
