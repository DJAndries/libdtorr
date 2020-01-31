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

static char* concat_path(dtorr_node* path) {
  char* result;
  unsigned long i;
  unsigned long cat_path_length = 0;
  for (i = 0; i < path->len; i++) {
    cat_path_length += ((dtorr_node**)path->value)[i]->len + 1;
  }
  result = (char*)malloc(sizeof(char) * cat_path_length);
  if (result == 0) {
    return 0;
  }
  result[0] = 0;
  for (i = 0; i < path->len; i++) {
    strcat(result, (char*)(((dtorr_node**)path->value)[i]->value));
    if (i < (path->len - 1)) {
      strcat(result, "/");
    }
  }
  return result;
}

static dtorr_file* construct_file(dtorr_config* config, unsigned long length, dtorr_node* path) {
  dtorr_file* file;
  char* cat_path = concat_path(path);
  if (cat_path == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Unable to concatenate file path");
    return 0;
  }
  file = (dtorr_file*)malloc(sizeof(dtorr_file));
  if (file == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Unable to allocate file struct");
    return 0;
  }
  file->length = length;
  file->path = path;
  file->cat_path = cat_path;
  return file;
}

static int validate_and_calc_files(dtorr_config* config, dtorr_torrent* result, dtorr_node* files) {
  unsigned long i, file_length;
  dtorr_node* node;
  dtorr_node* inner_node;

  dlog(config, LOG_LEVEL_DEBUG, "Metaloading: validate and build files");
  if (files == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Unable to get file hashmap entries");
    return 1;
  }

  result->files = (dtorr_file**)malloc(sizeof(dtorr_file*) * files->len);
  if (result->files == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Unable to allocate files array");
    return 5;
  } 

  for (i = 0; i < files->len; i++) {
    node = ((dtorr_node**)files->value)[i];
    if (node->type != DTORR_DICT) {
      dlog(config, LOG_LEVEL_ERROR, "File is not a dict");
      return 2;
    }

    inner_node = hashmap_get((dtorr_hashmap*)node->value, "length");
    if (inner_node == 0 || inner_node->type != DTORR_NUM || *((long*)inner_node->value) <= 0) {
      dlog(config, LOG_LEVEL_ERROR, "Invalid file length");
      return 3;
    }
    file_length = *((long*)inner_node->value);
    result->length += file_length;

    inner_node = hashmap_get((dtorr_hashmap*)node->value, "path");
    if (inner_node == 0 || inner_node->type != DTORR_LIST || inner_node->len == 0) {
      dlog(config, LOG_LEVEL_ERROR, "No file path");
      return 4;
    }

    result->files[i] = construct_file(config, file_length, inner_node);
    if (result->files[i] == 0) {
      return 6;
    }

    result->file_count++;
  }
  return 0;
}

static int validate_pieces_and_length(dtorr_config* config, dtorr_torrent* result) {
  unsigned long valid_min_length = result->piece_length * (result->piece_count - 1);
  unsigned long valid_max_length = result->piece_length * result->piece_count;

  dlog(config, LOG_LEVEL_DEBUG, "Metaloading: check piece length");
  if (result->length < valid_min_length || result->length > valid_max_length) {
    dlog(config, LOG_LEVEL_ERROR, "Piece length/count does not match length");
    return 1;
  }
  return 0;
}

static int generate_infohash(dtorr_config* config, dtorr_torrent* result, dtorr_node* info) {
  unsigned long encoded_len;
  char* encoded_info;

  dlog(config, LOG_LEVEL_DEBUG, "Infohash generation");

  encoded_info = bencoding_encode(config, info, &encoded_len);

  if (encoded_info == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Unable to bencode info dict");
    return 1;
  }

  dlog(config, LOG_LEVEL_DEBUG, "Encoded info, starting hash");

  if (SHA1((unsigned char*)encoded_info, encoded_len, (unsigned char*)result->infohash) == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Failed to generate infohash");
    free(encoded_info);
  }

  return 0;
}

static int process_info(dtorr_config* config, dtorr_torrent* result, dtorr_hashmap* info) {
  dtorr_node* node;

  dlog(config, LOG_LEVEL_DEBUG, "Metaloading: processing info");
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
    result->file_count = 1;
  }

  node = hashmap_get(info, "files");
  if (node != 0) {
    if (node->type != DTORR_LIST || node->len == 0) {
      dlog(config, LOG_LEVEL_ERROR, "Files is not a list, or is empty");
      return 4;
    }
    result->file_count = 0;
    if (validate_and_calc_files(config, result, node) != 0) {
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

  dlog(config, LOG_LEVEL_DEBUG, "Metaloading: processing decoded");
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
  unsigned long i;
  /* free nodes for "decoded" (if exists), will clear the rest */
  if (torrent->infohash != 0) {
    free(torrent->infohash);
  }
  if (torrent->files != 0) {
    for (i = 0; i < torrent->file_count; i++) {
      free(torrent->files[i]->cat_path);
      free(torrent->files[i]);
    }
    free(torrent->files);
  }
  free(torrent);
}