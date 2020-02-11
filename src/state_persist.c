#include "state_persist.h"
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"
#include "dtorr/bencoding_encode.h"
#include "dtorr/bencoding_decode.h"
#include "log.h"

#define STATE_KEY "dtorr-state"
#define DOWNLOAD_DIR_KEY "download-dir"
#define BITFIELD_KEY "bitfield"

char* save_state(dtorr_config* config, dtorr_torrent* torrent, unsigned long* result_len) {
  dtorr_hashmap* decoded = (dtorr_hashmap*)torrent->decoded->value;
  dtorr_hashmap* state;
  dtorr_node *download_dir_node, *bitfield_node;

  dtorr_node* state_node = (dtorr_node*)hashmap_get(decoded, STATE_KEY);
  if (state_node == 0) {
    if ((state = hashmap_init(16)) == 0) {
      return 0;
    }
    if ((state_node = (dtorr_node*)malloc(sizeof(dtorr_node))) == 0) {
      free(state);
      return 0;
    }
    state_node->type = DTORR_DICT;
    state_node->value = state;
    if (hashmap_insert(decoded, STATE_KEY, state_node) != 0) {
      free(state_node);
      free(state);
      return 0;
    }
  } else {
    if (state_node->type != DTORR_DICT) {
      dlog(config, LOG_LEVEL_ERROR, "State save: existing state is not dict");
      return 0;
    }
    state = (dtorr_hashmap*)state_node->value;
  }

  bitfield_node = (dtorr_node*)hashmap_get(state, BITFIELD_KEY);
  if (bitfield_node == 0) {
    if ((bitfield_node = (dtorr_node*)malloc(sizeof(dtorr_node))) == 0) {
      return 0;
    }
    bitfield_node->type = DTORR_STR;
    bitfield_node->len = torrent->piece_count;
    if ((bitfield_node->value = (char*)malloc(sizeof(char) * torrent->piece_count)) == 0) {
      free(bitfield_node);
      return 0;
    }
    if (hashmap_insert(state, BITFIELD_KEY, bitfield_node) != 0) {
      free(bitfield_node->value);
      free(bitfield_node);
      return 0;
    }
  } else if (bitfield_node->type != DTORR_STR || bitfield_node->len != torrent->piece_count) {
    dlog(config, LOG_LEVEL_ERROR, "State save: existing bitfield length does not match piece count");
    return 0;
  }
  memcpy(bitfield_node->value, torrent->bitfield, torrent->piece_count);

  if (torrent->download_dir == 0) {
    dlog(config, LOG_LEVEL_ERROR, "State save: download dir does not exist");
    return 0;
  }

  download_dir_node = (dtorr_node*)hashmap_get(state, DOWNLOAD_DIR_KEY);
  if (download_dir_node == 0) {
    if ((download_dir_node = (dtorr_node*)malloc(sizeof(dtorr_node))) == 0) {
      return 0;
    }
    download_dir_node->type = DTORR_STR;
    
    if (hashmap_insert(state, DOWNLOAD_DIR_KEY, download_dir_node) != 0) {
      free(download_dir_node->value);
      free(download_dir_node);
      return 0;
    }
  } else {
    free(download_dir_node->value);
  }

  download_dir_node->len = strlen(torrent->download_dir);
  if ((download_dir_node->value = (char*)malloc(sizeof(char) * download_dir_node->len + 1)) == 0) {
    return 0;
  }
  strcpy(download_dir_node->value, torrent->download_dir);

  return bencoding_encode(config, torrent->decoded, result_len);
}

int parse_state(dtorr_config* config, dtorr_torrent* torrent) {
  dtorr_hashmap* decoded = (dtorr_hashmap*)torrent->decoded->value;
  dtorr_hashmap* state;
  dtorr_node *state_node, *download_dir_node, *bitfield_node;

  state_node = hashmap_get(decoded, STATE_KEY);
  if (state_node == 0 || state_node->type != DTORR_DICT || state_node->value == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "State parse: Ignoring state dict");
    return 1;
  } else {
    state = (dtorr_hashmap*)state_node->value;
  }

  download_dir_node = (dtorr_node*)hashmap_get(state, DOWNLOAD_DIR_KEY);
  if (download_dir_node == 0 || download_dir_node->type != DTORR_STR || download_dir_node->value == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "State parse: Ignoring download dir");
    return 2;
  }

  if (torrent->download_dir != 0) {
    free(torrent->download_dir);
  }
  if ((torrent->download_dir = (char*)malloc(sizeof(char) * download_dir_node->len + 1)) == 0) {
    return 3;
  }
  strcpy(torrent->download_dir, (char*)download_dir_node->value);

  bitfield_node = (dtorr_node*)hashmap_get(state, BITFIELD_KEY);
  if (bitfield_node == 0 || bitfield_node->type != DTORR_STR ||
      bitfield_node->value == 0 || bitfield_node->len != torrent->piece_count) {
    dlog(config, LOG_LEVEL_DEBUG, "State parse: Ignoring bitfield");
    return 4;
  }
  memcpy(torrent->bitfield, bitfield_node->value, bitfield_node->len);

  return 0;
}
