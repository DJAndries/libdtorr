#include "manager.h"
#include "msg_out.h"
#include "msg_in.h"
#include "hashmap.h"
#include "handshake.h"
#include "stream.h"
#include "close.h"
#include "log.h"
#include "util.h"

#define MAX_CONNECTIONS 20
#define START_PEERS_INTERVAL 10000
#define MSG_BUF_SIZE 32 * 1024

int start_peers(dtorr_config* config, dtorr_torrent* torrent) {
  unsigned long i;
  dtorr_hashnode** peer_entries;
  dtorr_peer* peer;

  if (torrent->active_peer_count >= MAX_CONNECTIONS) {
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
    if (peer_handshake(config, torrent, peer) != 0 || send_bitfield(config, torrent, peer) != 0) {
      peer_close(config, torrent, peer, 1);
      continue;
    }
    dlog(config, LOG_LEVEL_DEBUG, "Handshake & bitfield send ok");
  }
  
  return 0;
}

int manage_torrent(dtorr_config* config, dtorr_torrent* torrent) {
  dtorr_listnode* it;
  dtorr_listnode* next;
  dtorr_peer* peer;
  char buf[MSG_BUF_SIZE];
  unsigned long msg_len;
  int read_result;

  unsigned long curr_time = get_time_ms();

  if ((curr_time - torrent->last_peerstart_time) >= START_PEERS_INTERVAL) {
    dlog(config, LOG_LEVEL_DEBUG, "Peerstarts at %lu ms", curr_time);
    if (start_peers(config, torrent) != 0) {
      return 1;
    }
    torrent->last_peerstart_time = curr_time;
  }

  for (it = torrent->active_peers; it != 0; it = next) {
    next = it->next;
    peer = (dtorr_peer*)it->value;

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