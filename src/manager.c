#include "dtorr/manager.h"
#include "msg_out.h"
#include "msg_in.h"
#include "hashmap.h"
#include "handshake.h"
#include "requester.h"
#include "stream.h"
#include "choke.h"
#include "close.h"
#include "piece_ex.h"
#include "tracker.h"
#include "log.h"
#include "util.h"

#define MAX_CONNECTIONS 20
#define START_PEERS_INTERVAL 5000
#define REQUESTER_INTERVAL 500
#define CHOKE_INTERVAL 750
#define ANNOUNCE_INTERVAL 1800 * 1000
#define METRICS_INTERVAL 3000
#define MSG_BUF_SIZE 32 * 1024

static int start_peers(dtorr_config* config, dtorr_torrent* torrent) {
  unsigned long long i;
  dtorr_hashnode** peer_entries;
  dtorr_peer* peer;

  if (torrent->active_peer_count >= MAX_CONNECTIONS || torrent->downloaded == torrent->length) {
    return 0;
  }

  dlog(config, LOG_LEVEL_DEBUG, "Trying to connect to more peers");

  peer_entries = hashmap_entries(torrent->peer_map, 0);
  if (peer_entries == 0) {
    return 1;
  }

  for (i = 0; i < torrent->peer_map->entry_count && torrent->active_peer_count < MAX_CONNECTIONS; i++) {
    peer = (dtorr_peer*)peer_entries[i]->value;
    if (peer->bad == 1 || peer->active == 1) {
      continue;
    }
    if (peer_send_handshake(config, torrent, peer) != 0) {
      peer_close(config, torrent, peer, 1);
      continue;
    }
    dlog(config, LOG_LEVEL_DEBUG, "Handshake & bitfield send ok");
  }

  return 0;
}

static void calc_metrics(dtorr_config* config, dtorr_torrent* torrent, unsigned long long interval_time) {
  torrent->download_rate = torrent->downloaded_interval / (interval_time / 1000.0);
  torrent->upload_rate = torrent->uploaded_interval / (interval_time / 1000.0);
  dlog(config, LOG_LEVEL_DEBUG, "Download rate: %lu kbps Upload rate: %lu kbps",
       torrent->download_rate / 1000, torrent->upload_rate / 1000);
  torrent->downloaded_interval = 0;
  torrent->uploaded_interval = 0;
}

static int interval_tasks(dtorr_config* config, dtorr_torrent* torrent) {
  unsigned long long curr_time = get_time_ms();
  unsigned long long metrics_time;

  if ((curr_time - torrent->last_peerstart_time) >= START_PEERS_INTERVAL) {
    dlog(config, LOG_LEVEL_DEBUG, "Peerstarts at %lu ms", curr_time);
    if (start_peers(config, torrent) != 0) {
      return 1;
    }
    torrent->last_peerstart_time = curr_time;
  }

  if ((curr_time - torrent->last_requester_time) >= REQUESTER_INTERVAL) {
    interest_update(config, torrent);
    if (send_requests(config, torrent) != 0) {
      return 2;
    }
    torrent->last_requester_time = curr_time;
  }

  if ((curr_time - torrent->last_choke_time) >= CHOKE_INTERVAL) {
    choke_update(config, torrent);
    torrent->last_choke_time = curr_time;
  }

  if ((curr_time - torrent->last_announce_time >= ANNOUNCE_INTERVAL)) {
    tracker_announce(config, torrent->announce, torrent);
    torrent->last_announce_time = curr_time;
  }

  metrics_time = curr_time - torrent->last_metrics_time;
  if (metrics_time >= METRICS_INTERVAL) {
    calc_metrics(config, torrent, metrics_time);
    torrent->last_metrics_time = curr_time;
  }

  return 0;
}

int manage_torrent(dtorr_config* config, dtorr_torrent* torrent) {
  dtorr_listnode* it;
  dtorr_listnode* next;
  dtorr_peer* peer;
  char buf[MSG_BUF_SIZE];
  unsigned long long msg_len;
  int read_result, resend_result;

  if (interval_tasks(config, torrent) != 0) {
    return 1;
  }

  for (it = torrent->active_peers; it != 0; it = next) {
    next = it->next;
    peer = (dtorr_peer*)it->value;

    if (peer->unsent_data != 0) {
      dlog(config, LOG_LEVEL_DEBUG, "Attempting resend of unsent data");
      resend_result = attempt_resend(peer);
      if (resend_result < 0) {
        dlog(config, LOG_LEVEL_ERROR, "Failed to resend unsent data");
        peer_close(config, torrent, peer, 0);
        continue;
      } else if (resend_result > 0) {
        dlog(config, LOG_LEVEL_DEBUG, "Send buffer full, after resend");
        continue;
      }
    } 

    if (pieces_send(config, peer, torrent) < 0) {
      return 3;
    }

    read_result = extract_sock_msg(peer->s, buf, MSG_BUF_SIZE, &msg_len);
    if (read_result != 0) {
      if (read_result >= 2) {
        peer_close(config, torrent, peer, read_result == 2);
      }
      continue;
    }

    if (process_msg(config, torrent, peer, buf, msg_len) != 0) {
      peer_close(config, torrent, peer, 1);
    }
  }

  return 0;
}
