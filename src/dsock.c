#include "dsock.h"
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
  #include <fcntl.h>
#endif

int dsock_init() {
  #ifdef _WIN32
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data);
  #else
    return 0;
  #endif
}

static SOCKET connect_helper(char* host, unsigned short port, char* schema) {
  SOCKET s;
  addrinfo hints;
  addrinfo* result;
  char port_str[16];

  if (port != 0) {
    sprintf(port_str, "%u", port);
  }

  memset(&hints, 0, sizeof(addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if (getaddrinfo(host, port != 0 ? port_str : schema, &hints, &result) != 0) {
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

SOCKET dsock_connect_uri(parsed_uri* uri) {
  return connect_helper(uri->hostname, uri->port, uri->schema);
}

SOCKET dsock_connect(char* host, unsigned short port) {
  return connect_helper(host, port, 0);
}

int dsock_set_sock_nonblocking(SOCKET s) {
  #ifdef _WIN32
    unsigned long mode = 1;
    return ioctlsocket(s, FIONBIO, &mode);
  #else
    int flags = fcntl(s, F_GETFL, 0);
    if (flags == -1) {
      return 1;
    }
    flags |= O_NONBLOCK;
    return fcntl(s, F_SETFL, flags);
  #endif
}

SOCKET dsock_close(SOCKET s) {
  #ifdef _WIN32
    closesocket(s);
  #else
    close(s);
  #endif
  return INVALID_SOCKET;
}

int dsock_clean() {
  #ifdef _WIN32
    return WSACleanup();
  #else
    return 0;
  #endif
}
