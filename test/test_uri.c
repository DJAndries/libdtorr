#include <stdio.h>
#include "uri.h"

#define URI_COUNT 7

char *uris[] = {
  "http://test.com/announce",
  "http://localhost:8080/announce",
  "http://localhost",
  "udp://test.localhost.com:9999/announce/announce",
  "udp://test.localhost.com:9999/a",
  "udp://test.localhost.com:9999/",
  "udp://test.localhost.com:9999//",
  "udp://test.localhost.com:9999"
};

int test_uri(char* uri) {
  parsed_uri* parsed = parse_uri(uri);

  if (parsed == 0) {
    printf("Failed to parse\n");
    return 1;
  }
  
  printf("Schema: %s\n", parsed->schema);
  printf("Hostname: %s\n", parsed->hostname);
  printf("Hostname with port: %s\n", parsed->hostname_with_port);
  if (parsed->port != 0) {
    printf("Port: %u\n", parsed->port);
  }
  printf("Rest: %s\n\n", parsed->rest);
  free_parsed_uri(parsed);
  return 0;
}

int main() {
  unsigned long long i;

  for (i = 0; i < URI_COUNT; i++) {
    if (test_uri(uris[i]) != 0) {
      return 1;
    }
  }

  printf("Done!\n");
  return 0;
}
