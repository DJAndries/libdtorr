#include "dtorr/state_persist.h"
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"
#include "dtorr/bencoding_encode.h"
#include "dtorr/bencoding_decode.h"
#include "log.h"

#define STATE_KEY "dtorr-state"
#define DOWNLOAD_DIR_KEY "download-dir"
#define BITFIELD_KEY "bitfield"
#define DOWNLOADED_KEY "downloaded"
#define UPLOADED_KEY "uploaded"

int save_bitfield(dtorr_config* config, dtorr_torrent* torrent, dtorr_hashmap* state) {
  dtorr_node* bitfield_node = (dtorr_node*)hashmap_get(state, BITFIELD_KEY);
  if (bitfield_node == 0) {
    if ((bitfield_node = (dtorr_node*)malloc(sizeof(dtorr_node))) == 0) {
      return 1;
    }
    bitfield_node->type = DTORR_STR;
    bitfield_node->len = torrent->piece_count;
    if ((bitfield_node->value = (char*)malloc(sizeof(char) * torrent->piece_count)) == 0) {
      free(bitfield_node);
      return 2;
    }
    if (hashmap_insert(state, BITFIELD_KEY, bitfield_node) != 0) {
      free(bitfield_node->value);
      free(bitfield_node);
      return 3;
    }
  } else if (bitfield_node->type != DTORR_STR || bitfield_node->len != torrent->piece_count) {
    dlog(config, LOG_LEVEL_ERROR, "State save: existing bitfield length does not match piece count");
    return 4;
  }
  memcpy(bitfield_node->value, torrent->bitfield, torrent->piece_count);
  return 0;
}

int save_download_dir(dtorr_config* config, dtorr_torrent* torrent, dtorr_hashmap* state) {
  dtorr_node *download_dir_node;
  if (torrent->download_dir == 0) {
    dlog(config, LOG_LEVEL_ERROR, "State save: download dir does not exist");
    return 1;
  }

  download_dir_node = (dtorr_node*)hashmap_get(state, DOWNLOAD_DIR_KEY);
  if (download_dir_node == 0) {
    if ((download_dir_node = (dtorr_node*)malloc(sizeof(dtorr_node))) == 0) {
      return 2;
    }
    download_dir_node->type = DTORR_STR;
    if (hashmap_insert(state, DOWNLOAD_DIR_KEY, download_dir_node) != 0) {
      free(download_dir_node->value);
      free(download_dir_node);
      return 3;
    }
  } else {
    free(download_dir_node->value);
  }

  download_dir_node->len = strlen(torrent->download_dir);
  if ((download_dir_node->value = (char*)malloc(sizeof(char) * download_dir_node->len + 1)) == 0) {
    return 4;
  }
  strcpy(download_dir_node->value, torrent->download_dir);
  return 0;
}

int save_data_count(dtorr_config* config, dtorr_torrent* torrent, dtorr_hashmap* state, char* key, unsigned long value) {
  dtorr_hashmap* decoded = (dtorr_hashmap*)torrent->decoded->value;
  dtorr_node *node;
  node = (dtorr_node*)hashmap_get(decoded, key);
  if (node == 0) {
    if ((node = (dtorr_node*)malloc(sizeof(dtorr_node))) == 0) {
      return 2;
    }
    node->type = DTORR_NUM;
    if ((node->value = malloc(sizeof(long))) == 0) {
      free(node);
      return 3;
    }
    if (hashmap_insert(state, key, node) != 0) {
      free(node->value);
      free(node);
      return 4;
    }
  }

    *((long*)node->value) = (long)value;
  return 0;
}

char* save_state(dtorr_config* config, dtorr_torrent* torrent, unsigned long* result_len) {
  dtorr_hashmap* decoded = (dtorr_hashmap*)torrent->decoded->value;
  dtorr_hashmap* state;

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


  if (save_bitfield(config, torrent, state) != 0) {
    return 0;
  }

  if (save_download_dir(config, torrent, state) != 0) {
    return 0;
  }

  if (save_data_count(config, torrent, state, DOWNLOADED_KEY, torrent->downloaded) != 0 ||
      save_data_count(config, torrent, state, UPLOADED_KEY, torrent->uploaded) != 0) {
    return 0;
  }

  return bencoding_encode(config, torrent->decoded, result_len);
}

int parse_state(dtorr_config* config, dtorr_torrent* torrent) {
  dtorr_hashmap* decoded = (dtorr_hashmap*)torrent->decoded->value;
  dtorr_hashmap* state;
  dtorr_node *state_node, *download_dir_node, *bitfield_node, *downloaded_node, *uploaded_node;

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

  downloaded_node = (dtorr_node*)hashmap_get(state, DOWNLOADED_KEY);
  if (downloaded_node == 0 || downloaded_node->type != DTORR_NUM || downloaded_node->value < 0) {
    dlog(config, LOG_LEVEL_DEBUG, "State parse: Ignoring downloaded count");
  } else {
    torrent->downloaded = (unsigned long)*((long*)downloaded_node->value);
  }

  uploaded_node = (dtorr_node*)hashmap_get(state, UPLOADED_KEY);
  if (uploaded_node == 0 || uploaded_node->type != DTORR_NUM || uploaded_node->value < 0) {
    dlog(config, LOG_LEVEL_DEBUG, "State parse: Ignoring uploaded count");
  } else {
    torrent->uploaded = (unsigned long)*((long*)uploaded_node->value);
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
