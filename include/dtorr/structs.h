#ifndef DTORR_STRUCTS_H
#define DTORR_STRUCTS_H

#define DTORR_DICT 1 /* dtorr_hashmap */
#define DTORR_STR 2 /* char* */
#define DTORR_LIST 3 /* dtorr_node** */
#define DTORR_NUM 4 /* long long */

#ifndef _WIN32
typedef int SOCKET;
#else
#include <winsock2.h>
#endif

struct dtorr_node {
  int type;
  void* value;
  unsigned long long len;
};
typedef struct dtorr_node dtorr_node;

struct dtorr_hashnode {
  char* key;
  void* value;
};
typedef struct dtorr_hashnode dtorr_hashnode;

struct dtorr_hashmap {
  dtorr_hashnode** elements;
  unsigned long long map_size;
  unsigned long long entry_count;
};
typedef struct dtorr_hashmap dtorr_hashmap;

typedef struct dtorr_listnode dtorr_listnode;
struct dtorr_listnode {
  void* value;
  dtorr_listnode* next;
};

struct dtorr_file {
  dtorr_node* path;
  char* cat_path;
  unsigned long long length;
};
typedef struct dtorr_file dtorr_file;

struct dtorr_peer {
  char ip[64];
  unsigned short port;
  char peer_id[20];
  SOCKET s;

  char active;
  char they_choked;
  char we_choked;
  char they_interested;
  char we_interested;
  char bad;

  char* bitfield;

  char* unsent_data;
  unsigned long long unsent_data_offset;
  unsigned long long unsent_data_length;

  dtorr_listnode* out_piece_requests;
  unsigned long long sent_request_count;
  unsigned long long total_out_request_count;

  dtorr_listnode* in_piece_requests;
  unsigned long long total_in_request_count;
  unsigned long long curr_in_piece_index;
  char* curr_in_piece;
};
typedef struct dtorr_peer dtorr_peer;

struct dtorr_torrent {
  char* announce;
  char* name;
  unsigned long long piece_length;
  char* pieces;
  unsigned long long piece_count;
  unsigned long long length;

  dtorr_file** files;
  unsigned long long file_count;

  char infohash[20];
  char* bitfield;

  dtorr_node* decoded;

  unsigned long long downloaded;
  unsigned long long download_rate;
  unsigned long long downloaded_interval;
  unsigned long long uploaded;
  unsigned long long upload_rate;
  unsigned long long uploaded_interval;

  dtorr_hashmap* tracker_interval_map;
  dtorr_hashmap* peer_map;
  char** in_piece_buf_map;

  dtorr_peer me;
  dtorr_listnode* active_peers;
  unsigned long long active_peer_count;

  char* download_dir;
  unsigned long long last_peerstart_time;
  unsigned long long last_requester_time;
  unsigned long long last_choke_time;
  unsigned long long last_announce_time;
  unsigned long long last_metrics_time;
};
typedef struct dtorr_torrent dtorr_torrent;

struct dtorr_config {
  int log_level;
  void (*log_handler)(int, char*);

  dtorr_torrent* (*torrent_lookup)(char*);

  unsigned short port;
  SOCKET serv_sock;
};
typedef struct dtorr_config dtorr_config;

struct dtorr_piece_request {
  unsigned long long index;
  unsigned long long begin;
  unsigned long long length;
  char request_sent;
};
typedef struct dtorr_piece_request dtorr_piece_request;

#endif
