#include "dsock.h"
#include <stdio.h>
#include <string.h>

int dsock_init() {
  #ifdef _WIN32
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data);
  #else
    return 0;
  #endif
}

SOCKET dsock_connect_uri(parsed_uri* uri) {
  SOCKET s;
  addrinfo hints;
  addrinfo* result;
  char port_str[16];

  if (uri->port != 0) {
    sprintf(port_str, "%u", uri->port);
  }

  memset(&hints, 0, sizeof(addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if (getaddrinfo(uri->hostname, uri->port != 0 ? port_str : uri->schema, &hints, &result) != 0) {
    return INVALID_SOCKET;
  }

  for (; result != 0; result = result->ai_next) {
    s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (s == INVALID_SOCKET) {
      return INVALID_SOCKET;
    } 

    if (connect(s, result->ai_addr, result->ai_addrlen) == 0) {
      return s;
    }

    dsock_close(s);
  }
  return INVALID_SOCKET;
}

void dsock_close(SOCKET s) {
  #ifdef _WIN32
    closesocket(s);
  #else
    close(s);
  #endif
}

int dsock_clean() {
  #ifdef _WIN32
    return WSACleanup();
  #else
    return 0;
  #endif
}