#ifndef DSOCK_H
#define DSOCK_H

#include "uri.h"
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #define addrinfo ADDRINFOA
  #define sockaddr_in SOCKADDR_IN
  #define sockaddr_in6 SOCKADDR_IN6
  #define in_addr IN_ADDR
#else
  #include <sys/socket.h>
  #define SOCKET int
  #define INVALID_SOCKET -1
#endif

int dsock_init();

SOCKET dsock_connect_uri(parsed_uri* uri);

void dsock_close(SOCKET s);

int dsock_clean();

#endif