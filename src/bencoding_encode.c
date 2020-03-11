#include "dtorr/bencoding_encode.h"
#include "log.h"
#include "hashmap.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RECURSION 16
#define BUFFER_UNIT 128

static int encode_helper(dtorr_config* config, unsigned long long level, dtorr_node* node, char** result, unsigned long long* result_len, unsigned long long* result_size);

static int resize_result(dtorr_config* config, char** result, unsigned long long* result_size, unsigned long long increase) {
  if (increase == 0) {
    increase = BUFFER_UNIT;
  }
  dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: attempting buffer resize from %lu bytes", *result_size);
  char* resized = (char*)realloc(*result, *result_size + increase);
  if (resized == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: failed to resize result");
    return 1;
  }
  *result = resized;
  *result_size += increase;
  dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: resized to %lu bytes", *result_size);
  return 0;
}

static int handle_str(dtorr_config* config, dtorr_node* node, char* str, char** result, unsigned long long* result_len, unsigned long long* result_size) {
  unsigned long long char_write;
  unsigned long long len;
  if (node != 0) {
    str = (char*)node->value;
    len = node->len;
  } else {
    len = strlen(str);
  }
  if (len + *result_len + 32 >= *result_size) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: string needs buffer resize. curr len: %lu", *result_len);
    if (resize_result(config, result, result_size, (len + *result_len + 32) - *result_size + 1) != 0) {
      return 1;
    }
  }
  char_write = sprintf(*result + *result_len, SPEC_LLU ":", len);
  if (char_write == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: failed to encode string");
    return 2;
  }
  *result_len += char_write;
  memcpy(*result + *result_len, str, len);
  *result_len += len;
  *(*result + *result_len) = 0;
  return 0;
}

static int handle_num(dtorr_config* config, dtorr_node* node, char** result, unsigned long long* result_len, unsigned long long* result_size) {
  long long* num = (long long*)node->value;
  unsigned long long char_write;
  if (*result_len + 64 >= *result_size) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: num needs buffer resize");
    if (resize_result(config, result, result_size, (*result_len + 64) - *result_size + 1) != 0) {
      return 1;
    }
  }
  char_write = sprintf(*result + *result_len, "i" SPEC_LLD "e", *num);
  if (char_write == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: failed to encode string");
    return 2;
  }
  *result_len += char_write;
  return 0;
}

static int handle_list(dtorr_config* config, unsigned long long level, dtorr_node* node, char** result, unsigned long long* result_len, unsigned long long* result_size) {
  dtorr_node** list = (dtorr_node**)node->value;
  dtorr_node* element;
  unsigned long long i;

  if (*result_len + 8 >= *result_size) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: list needs buffer resize");
    if (resize_result(config, result, result_size, (*result_len + 8) - *result_size + 1) != 0) {
      return 1;
    }
  }

  *(*result + (*result_len)++) = 'l';
  for (i = 0; i < node->len; i++) {
    element = list[i];

    if (encode_helper(config, level + 1, element, result, result_len, result_size) != 0) {
      return 2;
    }
  }
  *(*result + (*result_len)++) = 'e';
  *(*result + (*result_len)) = 0;

  return 0;
}

static int handle_dict(dtorr_config* config, unsigned long long level, dtorr_node* node, char** result, unsigned long long* result_len, unsigned long long* result_size) {
  dtorr_hashmap* map = (dtorr_hashmap*)node->value;
  dtorr_hashnode** entries;
  dtorr_hashnode* entry;
  unsigned long long i;

  if (*result_len + 8 >= *result_size) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: dict needs buffer resize");
    if (resize_result(config, result, result_size, (*result_len + 8) - *result_size + 1) != 0) {
      return 1;
    }
  }

  entries = hashmap_entries(map, 1);
  if (entries == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: failed to get hashmap entries");
    return 2;
  }


  dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: encoding hashmap");

  *(*result + (*result_len)++) = 'd';
  for (i = 0; i < map->entry_count; i++) {
    entry = entries[i];
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: encoding hashmap key: %s", entry->key);

    if (handle_str(config, 0, entry->key, result, result_len, result_size) != 0) {
      free(entries);
      return 3;
    }
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: wrote hashmap key: %s", entry->key);
    if (encode_helper(config, level + 1, entry->value, result, result_len, result_size) != 0) {
      free(entries);
      return 4;
    }
  }
  *(*result + (*result_len)++) = 'e';
  *(*result + (*result_len)) = 0;

  return 0;
}

static int encode_helper(dtorr_config* config, unsigned long long level, dtorr_node* node, char** result, unsigned long long* result_len, unsigned long long* result_size) {
  int ret = 0;
  if (level > MAX_RECURSION) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: max recursion reached");
    return 0;
  }
  switch (node->type) {
    case DTORR_DICT:
      ret = handle_dict(config, level, node, result, result_len, result_size);
      break;
    case DTORR_LIST:
      ret = handle_list(config, level, node, result, result_len, result_size);
      break;
    case DTORR_NUM:
      ret = handle_num(config, node, result, result_len, result_size);
      break;
    case DTORR_STR:
      ret = handle_str(config, node, 0, result, result_len, result_size);
      break;
    default:
      dlog(config, LOG_LEVEL_DEBUG, "Bencoding encode: unknown node type");
      return 0;
  }
  return ret;
}

char* bencoding_encode(dtorr_config* config, dtorr_node* node, unsigned long long* result_len) {
  unsigned long long result_size = BUFFER_UNIT;
  char* result = (char*)malloc(sizeof(char) * result_size);
  memset(result, 0, result_size);

  *result_len = 0;

  if (encode_helper(config, 1, node, &result, result_len, &result_size) != 0) {
    dlog(config, LOG_LEVEL_ERROR, "Bencoding encode: unable to encode");
    free(result);
    return 0;
  }

  return result;
}
