#include "requester.h"
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "list.h"
#include "close.h"
#include "msg_out.h"
#include "util.h"

#define MAX_SENT_REQUESTS 300
#define REQUEST_SIZE 16 * 1024

long long bitfield_interest_index(dtorr_torrent* torrent, dtorr_peer* peer, char random) {
  long long index = 0;
  long long i;

  if (random == 1) {
    srand((unsigned int)get_time_ms());
    index = rand() % torrent->piece_count;
  }

  for (i = 0; i < torrent->piece_count; i++) {
    if (torrent->bitfield[index] == 0 && peer->bitfield[index] == 1
      && torrent->in_piece_buf_map[index] == 0) {
      return index;
    }
    index = (index + 1) % torrent->piece_count;
  }
  return -1;
}

void interest_update(dtorr_config* config, dtorr_torrent* torrent) {
  long long interest_index;
  dtorr_listnode *it, *next;
  dtorr_peer* peer;
  char interested;
  for (it = torrent->active_peers; it != 0; it = next) {
    peer = (dtorr_peer*)it->value;
    next = it->next;
    interested = -1;

    interest_index = bitfield_interest_index(torrent, peer, 0);

    if (interest_index == -1 && peer->we_interested == 1 && peer->out_piece_requests == 0) {
      interested = 0;
    } else if (interest_index != -1 && peer->we_interested == 0) {
      interested = 1;
    }

    if (interested != -1) {
      peer->we_interested = interested;
      if (send_interested_status(config, peer, interested) != 0) {
        peer_close(config, torrent, peer, 0);
        continue;
      }
      peer->we_interested = interested;
      dlog(config, LOG_LEVEL_DEBUG, "Sent interested status: %d", interested);
    }
  }
}

static int queue_requests(dtorr_config* config, dtorr_torrent* torrent, dtorr_peer* peer) {
  long long request_index;
  unsigned long long piece_length, piece_length_i, begin;
  dtorr_piece_request* req;
  if (peer->total_out_request_count >= MAX_SENT_REQUESTS * 2) {
    return 0;
  }

  if ((request_index = bitfield_interest_index(torrent, peer, 1)) == -1) {
    return 0;
  }

  dlog(config, LOG_LEVEL_DEBUG, "Queueing requests for piece %ld", request_index);
  piece_length_i = piece_length = calc_piece_length(torrent->piece_count, torrent->piece_length, torrent->length, request_index);

  begin = 0;
  while (piece_length_i != 0) {
    req = (dtorr_piece_request*)malloc(sizeof(dtorr_piece_request));
    if (req == 0) {
      return 1;
    }
    memset(req, 0, sizeof(dtorr_piece_request));
    req->length = piece_length_i >= REQUEST_SIZE ? REQUEST_SIZE : piece_length_i;
    req->index = request_index;
    req->begin = begin;
    begin += req->length;
    piece_length_i -= req->length;
    if (list_insert(&peer->out_piece_requests, req) != 0) {
      free(req);
      return 2;
    }
    peer->total_out_request_count++;
  }

  torrent->in_piece_buf_map[request_index] = malloc(sizeof(char) * piece_length);
  if (torrent->in_piece_buf_map[request_index] == 0) {
    return 3;
  }

  return queue_requests(config, torrent, peer);
}

int send_requests(dtorr_config* config, dtorr_torrent* torrent) {
  dtorr_listnode *it, *next;
  dtorr_listnode* req_it;
  dtorr_peer* peer;
  dtorr_piece_request* req;
  for (it = torrent->active_peers; it != 0; it = next) {
    next = it->next;
    peer = (dtorr_peer*)it->value;
    if (peer->they_choked == 1) {
      continue;
    }

    if (queue_requests(config, torrent, peer) != 0) {
      return 1;
    }

    for (req_it = peer->out_piece_requests; req_it != 0; req_it = req_it->next) {
      if (((dtorr_piece_request*)req_it->value)->request_sent == 0) {
        break;
      }
    }

    for (; peer->sent_request_count < MAX_SENT_REQUESTS && req_it != 0; peer->sent_request_count++) {
      req = (dtorr_piece_request*)req_it->value;
      dlog(config, LOG_LEVEL_DEBUG, "Sending request. piece: %lu begin: %lu len: %lu",
        req->index, req->begin, req->length);
      if (send_request(config, torrent, peer, req) != 0) {
        break;
      }
      req->request_sent = 1;
      req_it = req_it->next;
    }
  }
  return 0;
}
