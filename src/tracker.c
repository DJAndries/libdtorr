#include "tracker.h"
#include "dsock.h"
#include "log.h"
#include "uri.h"
#include "dtorr/bencoding_decode.h"
#include "hashmap.h"
#include <stdio.h>
#include <string.h>

#define RECV_BUFF_SIZE 64 * 1024
#define SEND_BUFF_SIZE 4096
#define HASHMAP_SIZE 512

static int process_peers(dtorr_config* config, dtorr_torrent* torrent, char* uri, dtorr_node* peer_str) {
  unsigned long i;
  dtorr_peer* peer;
  char ip_and_port[128];

  if (peer_str == 0 || peer_str->type != DTORR_STR || peer_str->len % 6 != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Tracker announce: peers string not available or is not compact format");
    return 1;
  }
  for (i = 0; i < peer_str->len; i += 6) {
    peer = (dtorr_peer*)malloc(sizeof(dtorr_peer));
    if (peer == 0) {
      dlog(config, LOG_LEVEL_ERROR, "Tracker announce: failed to allocate peer"); 
      return 2;
    }
    memset(peer, 0, sizeof(dtorr_peer));
    strcpy(peer->ip, inet_ntoa(*((in_addr*)(peer_str->value + i))));
    
    peer->port = *((char*)peer_str->value + i + 4) << 8;
    peer->port |= *((char*)peer_str->value + i + 5);

    /* compare ip in prod */
    if (peer->port == torrent->me.port) {
      free(peer);
      continue;
    }
    
    sprintf(ip_and_port, "%s:%u", peer->ip, peer->port);
    if (hashmap_get(torrent->peer_map, ip_and_port) == 0) {
      dlog(config, LOG_LEVEL_DEBUG, "Tracker announce: discovered peer %s", ip_and_port);
      if (hashmap_insert(torrent->peer_map, ip_and_port, peer) != 0) {
        dlog(config, LOG_LEVEL_ERROR, "Tracker announce: failed to place peer in map"); 
        free(peer);
        return 3;
      }
    }
  }
  return 0;
}

static int process_dict(dtorr_config* config, dtorr_torrent* torrent, char* uri, dtorr_node* response_dict) {
  dtorr_node* node;
  long* interval;

  if (response_dict->type != DTORR_DICT) {
    dlog(config, LOG_LEVEL_ERROR, "Tracker announce: response is not dict");
    return 1;
  }

  node = hashmap_get(response_dict->value, "failure reason");
  if (node != 0) {
    if (node->type == DTORR_STR) {
      dlog(config, LOG_LEVEL_ERROR, "Tracker announce: Failed reason: %s", ((dtorr_node*)node->value)->value);
    }
    return 2;
  }

  interval = (long*)malloc(sizeof(long));
  node = hashmap_get(response_dict->value, "interval");
  if (node != 0 && node->type == DTORR_NUM && *((long*)node->value) > 0) {
    *interval = *((long*)node->value);
  } else {
    *interval = 1800;
  }

  node = hashmap_get(torrent->tracker_interval_map, uri);
  if (node != 0) {
    *((long*)node->value) = *interval;
    /* no longer need this, since one exists in the map */
    free(interval);
  } else {
    if (hashmap_insert(torrent->tracker_interval_map, uri, interval) != 0) {
      dlog(config, LOG_LEVEL_ERROR, "Tracker announce: Failed to insert interval in tracker map");
      free(interval);
      return 3;
    }
  }

  node = hashmap_get(response_dict->value, "peers");
  if (process_peers(config, torrent, uri, node) != 0) {
    return 4;
  }

  return 0;
}

static int process_response(dtorr_config* config, dtorr_torrent* torrent, char* uri, char* recvbuf, unsigned long recvlen) {
  dtorr_node* response_dict;
  char* response_dict_txt;

  if (strstr(recvbuf, "200 OK") == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Tracker announce: Bad HTTP response");
    return 1;
  }

  response_dict_txt = strstr(recvbuf, "\r\n\r\n");
  if (response_dict_txt == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Tracker announce: Failed to get dict start ptr");
    return 2;
  }
  response_dict_txt += 4;

  response_dict = bencoding_decode(config, response_dict_txt, recvlen - (response_dict_txt - recvbuf));

  if (torrent->tracker_interval_map == 0) {
    torrent->tracker_interval_map = hashmap_init(HASHMAP_SIZE);
  }
  if (torrent->peer_map == 0) {
    torrent->peer_map = hashmap_init(HASHMAP_SIZE);
  }
  if (torrent->tracker_interval_map == 0 || torrent->peer_map == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Tracker announce: Failed to init peer or tracker interval maps");
    /* free node */
    return 3;
  }

  if (process_dict(config, torrent, uri, response_dict) != 0) {
    /* free node*/
    return 4;
  }

  /* free node */
  return 0;
}

static void generate_get_keys(dtorr_config* config, dtorr_torrent* torrent, char* result) {
  unsigned long i;
  char tmp[16];
  char info_hash[256] = "";
  char peer_id[256] = "";
  for (i = 0; i < 20; i++) {
    sprintf(tmp, "%%%02X", torrent->infohash[i] & 0xFF);
    strcat(info_hash, tmp);

    sprintf(tmp, "%%%02X", torrent->me.peer_id[i] & 0xFF);
    strcat(peer_id, tmp);
  }

  sprintf(result, "?info_hash=%s&peer_id=%s&ip=%s&port=%u&uploaded=%lu&downloaded=%lu&left=%lu&compact=1",
    info_hash, peer_id, torrent->me.ip, torrent->me.port, torrent->uploaded, torrent->downloaded, torrent->length - torrent->downloaded);
}

static int send_and_receive(dtorr_config* config, parsed_uri* parsed, dtorr_torrent* torrent, char* recvbuf, unsigned long* recvlen) {
  SOCKET s;
  char sendbuf[SEND_BUFF_SIZE];
  unsigned long sendlen;
  char get_keys[1024];

  if ((s = dsock_connect_uri(parsed)) == INVALID_SOCKET) {
    dlog(config, LOG_LEVEL_ERROR, "Tracker announce connection failure: %s", parsed->hostname_with_port);
    free_parsed_uri(parsed);
    return 2;
  }

  generate_get_keys(config, torrent, get_keys);

  sendlen = sprintf(sendbuf, "GET %s%s\r\nHost: %s\r\nUser-Agent: dtorr\r\n\r\n", parsed->rest, get_keys, parsed->hostname_with_port);
  if (send(s, sendbuf, sendlen, 0) != sendlen) {
    dlog(config, LOG_LEVEL_ERROR, "Tracker announce: failed to send");
    free_parsed_uri(parsed);
    dsock_close(s);
    return 3;
  }
  
  free_parsed_uri(parsed);

  *recvlen = recv(s, recvbuf, RECV_BUFF_SIZE, MSG_WAITALL);
  if (*recvlen == 0 || *recvlen == RECV_BUFF_SIZE) {
    dlog(config, LOG_LEVEL_ERROR, "Tracker announce recv failure. Did not receive or received too much");
    dsock_close(s);
    return 4;
  }

  recvbuf[RECV_BUFF_SIZE - 1] = 0;

  dsock_close(s);
  return 0;
}

int tracker_announce(dtorr_config* config, char* uri, dtorr_torrent* torrent) {
  parsed_uri* parsed = parse_uri(uri);
  char recvbuf[RECV_BUFF_SIZE];
  unsigned long recvlen;

  if (parsed == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Tracker announce failed to parse uri: %s", uri);
    free_parsed_uri(parsed);
    return 1;
  }

  if (send_and_receive(config, parsed, torrent, recvbuf, &recvlen)) {
    return 2;
  }

  if (process_response(config, torrent, uri, recvbuf, recvlen) != 0) {
    return 3;
  }

  return 0;
}