#ifndef URI_H
#define URI_H

#include "dsock.h"

struct parsed_uri {
  char* hostname;
  unsigned short port;
  char* hostname_with_port;
  char* schema;
  char* rest;
};
typedef struct parsed_uri parsed_uri;

parsed_uri* parse_uri(char* uri);

void free_parsed_uri(parsed_uri* uri);


#endif