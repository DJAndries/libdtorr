#include "dtorr/metadata.h"
#include "dtorr/structs.h"
#include "dtorr/bencoding_decode.h"
#include "dtorr/bencoding_encode.h"
#include "log.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

char *default_torrent_name = "torrent";

static int validate_and_calc_files(dtorr_config* config, dtorr_torrent* result) {
  unsigned long i;
  dtorr_node* files = result->files;
  dtorr_node* node;
  dtorr_node* inner_node;
  if (files == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Unable to get file hashmap entries");
    return 1;
  }

  for (i = 0; i < files->len; i++) {
    node = ((dtorr_node**)files->value)[i];

    if (node->type == DTORR_DICT) {
      dlog(config, LOG_LEVEL_ERROR, "File is not a dict");
      return 2;
    }

    inner_node = hashmap_get((dtorr_hashmap*)node->value, "length");
    if (inner_node == 0 || inner_node->type == DTORR_NUM || *((long*)inner_node->value) <= 0) {
      dlog(config, LOG_LEVEL_ERROR, "Invalid file length");
      return 3;
    }
    result->length += *((long*)inner_node->value);

    inner_node = hashmap_get((dtorr_hashmap*)node->value, "path");
    if (inner_node == 0 || inner_node->type != DTORR_STR || inner_node->len == 0) {
      dlog(config, LOG_LEVEL_ERROR, "No file path");
      return 4;
    }
  }
  return 0;
}

static int validate_pieces_and_length(dtorr_config* config, dtorr_torrent* result) {
  unsigned long valid_min_length = result->piece_length * (result->piece_count - 1);
  unsigned long valid_max_length = result->piece_length * result->piece_count;
  if (result->length < valid_min_length || result->length > valid_max_length) {
    dlog(config, LOG_LEVEL_ERROR, "Piece length/count does not match length");
    return 1;
  }
  return 0;
}

static int generate_infohash(dtorr_config* config, dtorr_torrent* result, dtorr_node* info) {
  unsigned long encoded_len;
  char* encoded_info;
  char* hash;

  dlog(config, LOG_LEVEL_DEBUG, "Infohash generation");

  encoded_info = bencoding_encode(config, info, &encoded_len);

  if (encoded_info == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Unable to bencode info dict");
    return 1;
  }

  hash = (char*)malloc(sizeof(char) * 20);
  if (hash == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Unable to allocate mem for infohash");
    free(encoded_info);
    return 2;
  }

  dlog(config, LOG_LEVEL_DEBUG, "Encoded info, starting hash");

  if (SHA1((unsigned char*)encoded_info, encoded_len, (unsigned char*)hash) == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Failed to generate infohash");
    free(hash);
    free(encoded_info);
  }

  result->infohash = hash;

  return 0;
}

static int process_info(dtorr_config* config, dtorr_torrent* result, dtorr_hashmap* info) {
  dtorr_node* node;

  node = hashmap_get(info, "name");
  if (node != 0 && node->type == DTORR_STR) {
    result->name = (char*)node->value;
  } else {
    result->name = default_torrent_name;
  }

  node = hashmap_get(info, "pieces");
  if (node == 0 || node->type != DTORR_STR || node->len % 20 != 0) {
    dlog(config, LOG_LEVEL_ERROR, "No pieces available or not right length");
    return 1;
  }
  result->pieces = node;

  node = hashmap_get(info, "piece length");
  if (node == 0 || node->type != DTORR_NUM || *((long*)node->value) <= 0) {
    dlog(config, LOG_LEVEL_ERROR, "Invalid piece length");
    return 2;
  }
  result->piece_length = *((long*)node->value);

  node = hashmap_get(info, "length");
  if (node != 0) {
    if (node->type != DTORR_NUM || *((long*)node->value) <= 0) {
      dlog(config, LOG_LEVEL_ERROR, "Invalid length");
      return 3;
    }
    result->length = *((long*)node->value);
  }

  node = hashmap_get(info, "files");
  if (node != 0) {
    if (node->type == DTORR_LIST || node->len == 0) {
      dlog(config, LOG_LEVEL_ERROR, "Files is not a list, or is empty");
      return 4;
    }
    result->files = node;
    result->length = validate_and_calc_files(config, result);
    if (result->length == 0) {
      return 5;
    }
  }

  if (result->length == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Length is zero");
    return 6;
  }

  result->piece_count = result->pieces->len / 20;
  if (validate_pieces_and_length(config, result) != 0) {
    return 7;
  }

  return 0;
}

static int process_decoded(dtorr_config* config, dtorr_torrent* result, dtorr_node* decoded) {
  dtorr_node* node;

  result->decoded = decoded;

  if (decoded->type != DTORR_DICT) {
    dlog(config, LOG_LEVEL_ERROR, "Root torrent metadata node is not dict");
    return 1;
  }

  node = hashmap_get((dtorr_hashmap*)decoded->value, "announce");
  if (node == 0 || node->len == 0 || node->type != DTORR_STR) {
    dlog(config, LOG_LEVEL_ERROR, "Cannot find announce URL or not string");
    return 2;
  }
  result->announce = (char*)node->value;

  node = hashmap_get((dtorr_hashmap*)decoded->value, "info");
  if (node == 0 || node->type != DTORR_DICT) {
    dlog(config, LOG_LEVEL_ERROR, "Cannot find info dict");
    return 3;
  }
  if (process_info(config, result, (dtorr_hashmap*)node->value) != 0) {
    return 4;
  }

  if (generate_infohash(config, result, node) != 0) {
    return 5;
  }

  return 0;
}

dtorr_torrent* load_torrent_metadata(dtorr_config* config, char* data, unsigned long data_len) {
  dtorr_torrent* result = (dtorr_torrent*)malloc(sizeof(dtorr_torrent));
  dtorr_node* decoded_data;
  if (result == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Cannot alloc memory for torrent metadata");
    return 0;
  }
  memset(result, 0, sizeof(dtorr_torrent));

  decoded_data = bencoding_decode(config, data, data_len);
  if (decoded_data == 0) {
    free_torrent(result);
    return 0;
  }

  if (process_decoded(config, result, decoded_data) != 0) {
    free_torrent(result);
    return 0;
  } 

  return result;
}

void free_torrent(dtorr_torrent* torrent) {
  /* free nodes for "decoded" (if exists), will clear the rest */
  if (torrent->infohash != 0) {
    free(torrent->infohash);
  }
  free(torrent);
}