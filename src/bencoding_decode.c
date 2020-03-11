#include "dtorr/bencoding_decode.h"
#include "dtorr/structs.h"
#include "hashmap.h"
#include "log.h"
#include "util.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_RECURSION 16

static char* decode_helper(dtorr_config* config, unsigned long long level, dtorr_node* curr_node, char* value, char* value_end);

static char* handle_str(dtorr_config* config, dtorr_node* curr_node, char* value, char* value_end, char** extracted_str) {
  unsigned long long str_length;
  char* str_value;

  if (sscanf(value, SPEC_LLU ":", &str_length) == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unable to parse str length");
    return 0;
  }

  value = strchr(value, ':');
  if (value == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unable to find str length separator");
    return 0;
  }
  value++;

  if (str_length > (value_end - value)) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: defined str length is greater than actual length");
    return 0;
  }

  str_value = (char*)malloc(sizeof(char) * str_length + 1);
  if (str_value == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unable to allocate str");
    return 0;
  }

  dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: str attempting copy. len: %lu", str_length);

  memcpy(str_value, value, str_length);
  str_value[str_length] = 0;

  if (curr_node != 0) {
    curr_node->value = str_value;
    curr_node->type = DTORR_STR;
    curr_node->len = str_length;
  }
  if (extracted_str != 0) {
    *extracted_str = str_value;
  }
  value += str_length;

  dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: created string with len %lu", str_length);

  return value;
}

static char* handle_dict(dtorr_config* config, unsigned long long level, dtorr_node* curr_node, char* value, char* value_end) {
  char* key = 0;
  dtorr_node* node = 0;
  dtorr_hashmap* map = hashmap_init(256);

  if (map == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unable to init hashmap");
    return 0;
  }

  curr_node->value = map;
  curr_node->len = 0;
  curr_node->type = DTORR_DICT;

  dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: created dict");

  while (value != 0 && value[0] != 'e') {
    if (value[0] == 0) {
      dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unexpected end when parsing dict");
      return 0;
    }

    if (key == 0) {
      value = handle_str(config, 0, value, value_end, &key);
      dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: processed key string %s", key);
      if (value == 0 || key == 0) {
        return 0;
      }

      node = (dtorr_node*)malloc(sizeof(dtorr_node));
      if (node == 0) {
        dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unable to init hashmap value node");
        return 0;
      }

      memset(node, 0, sizeof(dtorr_node));
      if (hashmap_insert(map, key, node) != 0) {
        dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unable to insert in hashmap");
        free(node);
        return 0;
      }

      dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: inserted dict key \"%s\"", key);
    } else {
      value = decode_helper(config, level + 1, node, value, value_end);
      if (value == 0) {
        return 0;
      }
      dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: decoded dict value for key \"%s\"", key);
      key = 0;
      node = 0;
    }
  }
  
  return value + 1;
}

static char* handle_list(dtorr_config* config, unsigned long long level, dtorr_node* curr_node, char* value, char* value_end) {
  unsigned long long size = 256;
  dtorr_node** list = (dtorr_node**)malloc(sizeof(dtorr_node*) * size);
  dtorr_node* element;
  
  if (list == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unable to allocate list");
    return 0;
  }

  curr_node->value = list;
  curr_node->type = DTORR_LIST;
  curr_node->len = 0;

  dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: created list");

  while (value != 0 && value[0] != 'e') {
    if (value[0] == 0) {
      dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unexpected end when parsing list");
      return 0;
    }
    element = (dtorr_node*)malloc(sizeof(dtorr_node));
    if (element == 0) {
      dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unable to allocate list node");
      free(element);
      return 0;
    }
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: allocated list element");
    memset(element, 0, sizeof(dtorr_node));
    if (curr_node->len == size) {
      size += 256;
      list = (dtorr_node**)realloc(list, sizeof(dtorr_node*) * size);
      if (list == 0) {
        dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: failed to realloc list");
        free(element);
        return 0;
      }
      curr_node->value = list;
    }
    list[curr_node->len++] = element;
    value = decode_helper(config, level + 1, element, value, value_end);
    if (value == 0) {
      return 0;
    }
  }

  dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: list has %lu elements", curr_node->len);

  return value + 1;
}

static char* handle_num(dtorr_config* config, dtorr_node* curr_node, char* value) {
  long long* num = (long long*)malloc(sizeof(long long));

  if (num == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unable to allocate num");
    return 0;
  }

  if (sscanf(value, SPEC_LLD "e", num) == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unable to parse num");
    free(num);
    return 0;
  }

  value = strchr(value, 'e');
  if (value == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: unable to find num end character");
    free(num);
    return 0;
  }
  value++;

  curr_node->value = num;
  curr_node->type = DTORR_NUM;
  curr_node->len = 0;

  dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: created num");

  return value;
}

static char* decode_helper(dtorr_config* config, unsigned long long level, dtorr_node* curr_node, char* value, char* value_end) {
  if (level > MAX_RECURSION) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: max recursion reached");
    return 0;
  }
  char c = value[0];
  if (c == 'd') {
    value = handle_dict(config, level, curr_node, value + 1, value_end);
  } else if (c == 'i') {
    value = handle_num(config, curr_node, value + 1);
  } else if (c == 'l') {
    value = handle_list(config, level, curr_node, value + 1, value_end);
  } else if (c >= '0' && c <= '9') {
    value = handle_str(config, curr_node, value, value_end, 0);
  } else {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: invalid node heading");
    return 0;
  }
  return value;
}

dtorr_node* bencoding_decode(dtorr_config* config, char* value, unsigned long long value_len) {
  char* value_end = value + value_len;
  dtorr_node* result = (dtorr_node*)malloc(sizeof(dtorr_node));

  if (result == 0) {
    dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: failed to allocate initial resources");
    return 0;
  }

  memset(result, 0, sizeof(dtorr_node));

  if (decode_helper(config, 1, result, value, value_end) == 0) {
    dlog(config, LOG_LEVEL_ERROR, "Bencoding decode: failed to decode");
    /* TODO: free nodes */
    return 0;
  }
  dlog(config, LOG_LEVEL_DEBUG, "Bencoding decode: success");
  return result;
}
