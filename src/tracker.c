#include "tracker.h"
#include "dsock.h"
#include "log.h"
#include "uri.h"
#include "dtorr/bencoding_decode.h"
#include "hashmap.h"
#include "peer.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

#define RECV_BUFF_SIZE 256 * 1024
#define SEND_BUFF_SIZE 4096

static int process_peers(dtorr_config* config, dtorr_torrent* torrent, char* uri, dtorr_node* peer_str) {
  unsigned long long i;
  char ip[128];
  unsigned short port;

  if (peer_str == 0 || peer_str->type != DTORR_STR || peer_str->len % 6 != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Tracker announce: peers string not available or is not compact format");
    return 1;
  }
  for (i = 0; i < peer_str->len; i += 6) {
    strcpy(ip, inet_ntoa(*((in_addr*)(peer_str->value + i))));
    port = (*((char*)peer_str->value + i + 4) & 0xFF) << 8;
    port |= *((char*)peer_str->value + i + 5) & 0xFF;

    /* compare ip in prod */
    if (port == torrent->me.port) {
      continue;
    }

    if (peer_get_or_create(config, torrent, ip, port) == 0) {
      return 2;
    }
    dlog(config, LOG_LEVEL_DEBUG, "Tracker announce: discovered peer %s:%hu", ip, port);
  }
  return 0;
}

static int process_dict(dtorr_config* config, dtorr_torrent* torrent, char* uri, dtorr_node* response_dict) {
  dtorr_node* node;
  long long* interval;

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

  interval = (long long*)malloc(sizeof(long long));
  node = hashmap_get(response_dict->value, "interval");
  if (node != 0 && node->type == DTORR_NUM && *((long long*)node->value) > 0) {
    *interval = *((long long*)node->value);
  } else {
    *interval = 1800;
  }

  node = hashmap_get(torrent->tracker_interval_map, uri);
  if (node != 0) {
    *((long long*)node->value) = *interval;
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

static int process_response(dtorr_config* config, dtorr_torrent* torrent, char* uri, char* recvbuf, unsigned long long recvlen) {
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
    torrent->tracker_interval_map = hashmap_init(DEFAULT_HASHMAP_SIZE);
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
  unsigned long long i;
  char tmp[16];
  char info_hash[256] = "";
  char peer_id[256] = "";
  for (i = 0; i < 20; i++) {
    sprintf(tmp, "%%%02X", torrent->infohash[i] & 0xFF);
    strcat(info_hash, tmp);

    sprintf(tmp, "%%%02X", torrent->me.peer_id[i] & 0xFF);
    strcat(peer_id, tmp);
  }

  sprintf(result, "?info_hash=%s&peer_id=%s&ip=%s&port=%u&uploaded=" SPEC_LLU "&downloaded=" SPEC_LLU "&left=" SPEC_LLU "&compact=1",
    info_hash, peer_id, torrent->me.ip, torrent->me.port, torrent->uploaded, torrent->downloaded, torrent->length - torrent->downloaded);
}

static int send_and_receive(dtorr_config* config, parsed_uri* parsed, dtorr_torrent* torrent, char* recvbuf, unsigned long long* recvlen) {
  SOCKET s;
  char sendbuf[SEND_BUFF_SIZE];
  unsigned long long sendlen;
  char get_keys[1024];

  if ((s = dsock_connect_uri(parsed)) == INVALID_SOCKET) {
    dlog(config, LOG_LEVEL_ERROR, "Tracker announce connection failure: %s", parsed->hostname_with_port);
    free_parsed_uri(parsed);
    return 2;
  }

  generate_get_keys(config, torrent, get_keys);

  sendlen = sprintf(sendbuf, "GET %s%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: dtorr/1.0\r\n\r\n", parsed->rest, get_keys, parsed->hostname_with_port);
  if (send(s, sendbuf, sendlen, 0) != sendlen) {
    dlog(config, LOG_LEVEL_ERROR, "Tracker announce: failed to send");
    free_parsed_uri(parsed);
    dsock_close(s);
    return 3;
  }

  free_parsed_uri(parsed);

  *recvlen = dsock_recv_timeout(s, recvbuf, RECV_BUFF_SIZE, 7500);
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
  unsigned long long recvlen;

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
