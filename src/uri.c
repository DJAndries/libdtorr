#include "uri.h"
#include <string.h>
#include <stdlib.h>

unsigned long parse_schema(parsed_uri* result, char* uri, unsigned long len) {
  unsigned long schema_end_i;
  char* found_char = strchr(uri, ':');

  if (found_char == 0 || (schema_end_i = (found_char - uri)) > 20) {
    return 0;
  }

  result->schema = (char*)malloc(sizeof(char) * (schema_end_i + 1));
  if (result->schema == 0) {
    return 0;
  }
  memcpy(result->schema, uri, schema_end_i);
  result->schema[schema_end_i] = 0;

  if (len < (schema_end_i + 8) || uri[schema_end_i + 1] != '/' || uri[schema_end_i + 2] != '/') {
    return 0;
  }
  return schema_end_i;
}

unsigned long parse_hostname(parsed_uri* result, char* uri, unsigned long schema_end_i, unsigned long len) {
  unsigned long part_len, hostname_end_i;
  char* found_char = strchr(uri + schema_end_i + 4, '/');
  if (found_char == 0) {
    found_char = uri + len;
  }
  hostname_end_i = found_char - uri;
  part_len = hostname_end_i - (schema_end_i + 3);

  result->hostname_with_port = result->hostname = (char*)malloc(sizeof(char) * (part_len + 1));
  if (result->hostname == 0) {
    return 0;
  }
  memcpy(result->hostname, uri + schema_end_i + 3, part_len);
  result->hostname[part_len] = 0;
  return hostname_end_i;
}

int parse_port(parsed_uri* result) {
  char* port_buf;
  char* found_char = strrchr(result->hostname, ':');
  unsigned long buf_len, port_begin_i;

  if (found_char != 0 && *(found_char + 1) != 0) {
    port_begin_i = found_char - result->hostname + 1;
    buf_len = strlen(found_char + 1);
    port_buf = (char*)malloc(sizeof(char) * (buf_len + 1));
    if (port_buf == 0) {
      return 1;
    }

    memcpy(port_buf, result->hostname + port_begin_i, buf_len);
    port_buf[buf_len] = 0;
    result->port = strtoul(port_buf, 0, 0);
    free(port_buf);

    result->hostname = (char*)malloc(sizeof(char) * port_begin_i);
    if (result->hostname == 0) {
      return 2;
    }

    memcpy(result->hostname, result->hostname_with_port, port_begin_i - 1);
    result->hostname[port_begin_i - 1] = 0;

    if (result->port == 0) {
      return 3;
    }
  }
  return 0;
}

int parse_rest(parsed_uri* result, char* rest_start) {
  char use_default = 0;
  unsigned long len = strlen(rest_start);

  if (len < 2) {
    len = 1;
    use_default = 1;
  }

  result->rest = (char*)malloc(sizeof(char) * (len + 1));
  if (result->rest == 0) {
    return 2;
  }

  if (use_default == 1) {
    strcpy(result->rest, "/");
  } else {
    strcpy(result->rest, rest_start);
  }

  return 0;
} 

parsed_uri* parse_uri(char* uri) {
  parsed_uri* result;
  unsigned long schema_end_i, hostname_end_i;
  unsigned long len = strlen(uri);

  result = (parsed_uri*)malloc(sizeof(parsed_uri));
  memset(result, 0, sizeof(parsed_uri));

  if ((schema_end_i = parse_schema(result, uri, len)) == 0) {
    free_parsed_uri(result);
    return 0;
  }

  if ((hostname_end_i = parse_hostname(result, uri, schema_end_i, len)) == 0) {
    free_parsed_uri(result);
    return 0;
  }

  if (parse_port(result) != 0) {
    free_parsed_uri(result);
    return 0;
  }

  if (parse_rest(result, uri + hostname_end_i) != 0) {
    free_parsed_uri(result);
    return 0;
  }

  return result;
}

void free_parsed_uri(parsed_uri* result) {
  char* hostname_ptr = result->hostname;
  if (hostname_ptr != 0) {
    free(hostname_ptr);
  }
  if (result->hostname_with_port != 0 && (hostname_ptr == 0 || result->hostname_with_port != hostname_ptr)) {
    free(result->hostname_with_port);
  }
  if (result->schema != 0) {
    free(result->schema);
  }
  if (result->rest != 0) {
    free(result->rest);
  }
  free(result);
}