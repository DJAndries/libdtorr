#ifndef BENCODING_H
#define BENCODING_H

#define DTORR_DICT 1 /* dtorr_hashmap */
#define DTORR_STR 2 /* char* */
#define DTORR_LIST 3 /* dtorr_node** */
#define DTORR_NUM 4 /* long */

struct dtorr_node {
  int type;
  void* value;
  unsigned long len;
};
typedef struct dtorr_node dtorr_node;

struct dtorr_hashnode {
  char* key;
  dtorr_node* value;
};
typedef struct dtorr_hashnode dtorr_hashnode;

struct dtorr_hashmap {
  dtorr_hashnode** elements;
  unsigned long map_size;
  unsigned long entry_count;
};
typedef struct dtorr_hashmap dtorr_hashmap;

struct dtorr_config {
  int log_level;
  void (*log_handler)(int, char*);
};
typedef struct dtorr_config dtorr_config;

struct dtorr_ctx {
  dtorr_config* config;

};
typedef struct dtorr_ctx dtorr_ctx;

struct dtorr_file {
  dtorr_node* path;
  char* cat_path;
  unsigned long length;
};
typedef struct dtorr_file dtorr_file;

struct dtorr_torrent {
  char* announce;
  char* name;
  unsigned long piece_length;
  dtorr_node* pieces;
  unsigned long piece_count;
  unsigned long length;

  dtorr_file** files;
  unsigned long file_count;

  char* infohash;

  dtorr_node* decoded;
};
typedef struct dtorr_torrent dtorr_torrent;

#endif